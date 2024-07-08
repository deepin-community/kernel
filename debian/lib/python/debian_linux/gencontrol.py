from __future__ import annotations

import contextlib
import itertools
import pathlib
import re
from collections import OrderedDict
from collections.abc import (
    Generator,
)
from typing import (
    Any,
    Iterable,
    Iterator,
    IO,
)

from .config_v2 import (
    ConfigMerged,
    ConfigMergedDebianarch,
    ConfigMergedFeatureset,
    ConfigMergedFlavour,
)
from .debian import Changelog, PackageArchitecture, \
    Version, _ControlFileDict
from .utils import Templates


class PackagesList(OrderedDict):
    def append(self, package) -> None:
        self[package['Package']] = package

    def extend(self, packages) -> None:
        for package in packages:
            self[package['Package']] = package

    def setdefault(self, package) -> Any:
        return super().setdefault(package['Package'], package)


class Makefile:
    rules: dict[str, MakefileRule]

    def __init__(self) -> None:
        self.rules = {}

    def add_cmds(self, name: str, cmds) -> None:
        rule = self.rules.setdefault(name, MakefileRule(name))
        rule.add_cmds(MakefileRuleCmdsSimple(cmds))

    def add_deps(self, name: str, deps) -> None:
        rule = self.rules.setdefault(name, MakefileRule(name))
        rule.add_deps(deps)

        for i in deps:
            self.rules.setdefault(i, MakefileRule(i))

    def add_rules(self, name: str, target, makeflags, packages=set(), packages_extra=set()) -> None:
        rule = self.rules.setdefault(name, MakefileRule(name))
        rule.add_cmds(MakefileRuleCmdsRules(target, makeflags, packages, packages_extra))

    def write(self, out) -> None:
        out.write('''\
.NOTPARALLEL:
.PHONY:
packages_enabled := $(shell dh_listpackages)
define if_package
$(if $(filter $(1),$(packages_enabled)),$(2))
endef
''')
        for k, rule in sorted(self.rules.items()):
            rule.write(out)


class MakefileRule:
    name: str
    cmds: list[MakefileRuleCmds]
    deps: set[str]

    def __init__(self, name: str) -> None:
        self.name = name
        self.cmds = []
        self.deps = set()

    def add_cmds(self, cmds: MakefileRuleCmds) -> None:
        self.cmds.append(cmds)

    def add_deps(self, deps: Iterable[str]) -> None:
        self.deps.update(deps)

    def write(self, out: IO) -> None:
        if self.cmds:
            out.write(f'{self.name}:{" ".join(sorted(self.deps))}\n')
            for c in self.cmds:
                c.write(out)
        else:
            out.write(f'{self.name}:{" ".join(sorted(self.deps))}\n')


class MakefileRuleCmds:
    def write(self, out: IO) -> None:
        raise NotImplementedError


class MakefileRuleCmdsRules(MakefileRuleCmds):
    def __init__(self, target, makeflags, packages, packages_extra) -> None:
        self.target = target
        self.makeflags = makeflags.copy()
        self.packages = packages
        self.packages_extra = packages_extra

        packages_all = packages | packages_extra

        if packages_all:
            if len(packages_all) == 1:
                package_name = list(packages_all)[0]
                self.makeflags['PACKAGE_NAME'] = package_name
                self.makeflags['DESTDIR'] = f'$(CURDIR)/debian/{package_name}'
            else:
                self.makeflags['DESTDIR'] = '$(CURDIR)/debian/tmp'

            self.makeflags['DH_OPTIONS'] = ' '.join(f'-p{i}' for i in sorted(packages_all))

    def write(self, out: IO) -> None:
        cmd = f'$(MAKE) -f debian/rules.real {self.target} {self.makeflags}'
        if self.packages:
            out.write(f'\t$(call if_package, {" ".join(sorted(self.packages))}, {cmd})\n')
        else:
            out.write(f'\t{cmd}\n')


class MakefileRuleCmdsSimple(MakefileRuleCmds):
    cmds: list[str]

    def __init__(self, cmds: list[str]) -> None:
        self.cmds = cmds

    def write(self, out: IO) -> None:
        for i in self.cmds:
            out.write(f'\t{i}\n')


class MakeFlags(dict):
    def __str__(self) -> str:
        return ' '.join("%s='%s'" % i for i in sorted(self.items()))

    def copy(self) -> MakeFlags:
        return self.__class__(super(MakeFlags, self).copy())


class PackagesBundle:
    name: str | None
    templates: Templates
    base: pathlib.Path
    makefile: Makefile
    packages: PackagesList

    def __init__(
            self,
            name: str | None,
            templates: Templates,
            base: pathlib.Path = pathlib.Path('debian'),
    ) -> None:
        self.name = name
        self.templates = templates
        self.base = base
        self.makefile = Makefile()
        self.packages = PackagesList()

    def add(
            self,
            pkgid: str,
            ruleid: Iterable[str],
            makeflags: MakeFlags,
            replace: dict[str, str],
            *,
            arch: str | None = None,
            check_packages: bool = True,
    ) -> list[Any]:
        ret = []
        for raw_package in self.templates.get_control(f'{pkgid}.control', replace):
            package = self.packages.setdefault(raw_package)
            package_name = package['Package']
            ret.append(package)

            package.meta.setdefault('rules-ruleids', {})[ruleid] = makeflags
            if arch:
                package.meta.setdefault('architectures', PackageArchitecture()).add(arch)
            package.meta['rules-check-packages'] = check_packages

            for name in (
                    'NEWS',
                    'bug-presubj',
                    'lintian-overrides',
                    'maintscript',
                    'postinst',
                    'postrm',
                    'preinst',
                    'prerm',
            ):
                try:
                    template = self.templates.get(f'{pkgid}.{name}',
                                                  replace | {'package': package_name})
                except KeyError:
                    pass
                else:
                    with self.open(f'{package_name}.{name}') as f:
                        f.write(template)

        return ret

    def add_packages(
            self,
            packages: Iterable[_ControlFileDict],
            ruleid: Iterable[str],
            makeflags: MakeFlags,
            *,
            arch: str | None = None,
            check_packages: bool = True,
    ) -> None:
        for package in packages:
            package = self.packages.setdefault(package)
            package.meta.setdefault('rules-ruleids', {})[ruleid] = makeflags
            if arch:
                package.meta.setdefault('architectures', PackageArchitecture()).add(arch)
            package.meta['rules-check-packages'] = check_packages

    def path(self, name) -> pathlib.Path:
        if self.name:
            return self.base / f'generated.{self.name}/{name}'
        return self.base / name

    @staticmethod
    def __ruleid_deps(ruleid: tuple[str], name: str) -> Iterator[tuple[str, str]]:
        """
        Generate all the rules dependencies.
        ```
        build: build_a
        build_a: build_a_b
        build_a_b: build_a_b_image
        ```
        """
        r = ruleid + (name, )
        yield (
            '',
            '_' + '_'.join(r[:1]),
        )
        for i in range(1, len(r)):
            yield (
                '_' + '_'.join(r[:i]),
                '_' + '_'.join(r[:i + 1]),
            )

    @contextlib.contextmanager
    def open(self, name: str, mode: str = 'w') -> Generator[IO, None, None]:
        path = self.path(name)
        path.parent.mkdir(parents=True, exist_ok=True)
        with path.open(mode=mode, encoding='utf-8') as f:
            yield f

    def extract_makefile(self) -> None:
        targets: dict[frozenset[str], dict] = {}

        for package_name, package in self.packages.items():
            target_name = package.meta.get('rules-target')
            ruleids = package.meta.get('rules-ruleids')
            makeflags = MakeFlags({
                # Requires Python 3.9+
                k.removeprefix('rules-makeflags-').upper(): v
                for (k, v) in package.meta.items() if k.startswith('rules-makeflags-')
            })

            if ruleids:
                arches = package.meta.get('architectures')
                if arches:
                    package['Architecture'] = arches
                else:
                    arches = package.get('Architecture')

                if target_name:
                    for ruleid, makeflags_package in ruleids.items():
                        target_key = frozenset(
                            [target_name, ruleid]
                            + [f'{k}_{v}' for (k, v) in makeflags.items()]
                        )
                        target = targets.setdefault(
                            target_key,
                            {
                                'name': target_name,
                                'ruleid': ruleid,
                            },
                        )

                        if package.meta['rules-check-packages']:
                            target.setdefault('packages', set()).add(package_name)
                        else:
                            target.setdefault('packages_extra', set()).add(package_name)
                        makeflags_package = makeflags_package.copy()
                        makeflags_package.update(makeflags)
                        target['makeflags'] = makeflags_package

                        if arches == set(['all']):
                            target['type'] = 'indep'
                        else:
                            target['type'] = 'arch'

        for target in targets.values():
            name = target['name']
            ruleid = target['ruleid']
            packages = target.get('packages', set())
            packages_extra = target.get('packages_extra', set())
            makeflags = target['makeflags']
            ttype = target['type']

            rule = '_'.join(ruleid + (name, ))
            self.makefile.add_rules(f'setup_{rule}',
                                    f'setup_{name}', makeflags, packages, packages_extra)
            self.makefile.add_rules(f'build-{ttype}_{rule}',
                                    f'build_{name}', makeflags, packages, packages_extra)
            self.makefile.add_rules(f'binary-{ttype}_{rule}',
                                    f'binary_{name}', makeflags, packages, packages_extra)

            for i, j in self.__ruleid_deps(ruleid, name):
                self.makefile.add_deps(f'setup{i}',
                                       [f'setup{j}'])
                self.makefile.add_deps(f'build-{ttype}{i}',
                                       [f'build-{ttype}{j}'])
                self.makefile.add_deps(f'binary-{ttype}{i}',
                                       [f'binary-{ttype}{j}'])

    def merge_build_depends(self) -> None:
        # Merge Build-Depends pseudo-fields from binary packages into the
        # source package
        source = self.packages["source"]
        arch_all = PackageArchitecture("all")
        for name, package in self.packages.items():
            if name == "source":
                continue
            dep = package.get("Build-Depends")
            if not dep:
                continue
            del package["Build-Depends"]
            if package["Architecture"] == arch_all:
                dep_type = "Build-Depends-Indep"
            else:
                dep_type = "Build-Depends-Arch"
            for group in dep:
                for item in group:
                    if package["Architecture"] != arch_all and not item.arches:
                        item.arches = package["Architecture"]
                    if package.get("Build-Profiles") and not item.restrictions:
                        item.restrictions = package["Build-Profiles"]
                source.setdefault(dep_type).merge(group)

    def write(self) -> None:
        self.write_control()
        self.write_makefile()

    def write_control(self) -> None:
        with self.open('control') as f:
            self.write_rfc822(f, self.packages.values())

    def write_makefile(self) -> None:
        with self.open('rules.gen') as f:
            self.makefile.write(f)

    def write_rfc822(self, f: IO, entries: Iterable) -> None:
        for entry in entries:
            for key, value in entry.items():
                if value:
                    f.write(u"%s: %s\n" % (key, value))
            f.write('\n')


class Gencontrol(object):
    config: ConfigMerged
    vars: dict[str, str]
    bundles: dict[str, PackagesBundle]

    def __init__(self, config: ConfigMerged, templates, version=Version) -> None:
        self.config, self.templates = config, templates
        self.changelog = Changelog(version=version)
        self.vars = {}
        self.bundles = {'': PackagesBundle(None, templates)}

    @property
    def bundle(self) -> PackagesBundle:
        return self.bundles['']

    def __call__(self) -> None:
        self.do_source()
        self.do_main()
        self.do_extra()

        self.write()

    def do_source(self) -> None:
        source = self.templates.get_source_control("source.control", self.vars)[0]
        if not source.get('Source'):
            source['Source'] = self.changelog[0].source
        self.bundle.packages['source'] = source

    def do_main(self) -> None:
        vars = self.vars.copy()

        makeflags = MakeFlags()

        self.do_main_setup(self.config, vars, makeflags)
        self.do_main_makefile(self.config, vars, makeflags)
        self.do_main_packages(self.config, vars, makeflags)
        self.do_main_recurse(self.config, vars, makeflags)

    def do_main_setup(
        self,
        config: ConfigMerged,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_main_makefile(
        self,
        config: ConfigMerged,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_main_packages(
        self,
        config: ConfigMerged,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_main_recurse(
        self,
        config: ConfigMerged,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        for featureset in config.root_featuresets:
            self.do_indep_featureset(featureset, vars.copy(), makeflags.copy())

        # Sort the output the same way as before
        for arch in sorted(
            itertools.chain.from_iterable(
                i.debianarchs for i in config.kernelarchs
            ),
            key=lambda i: i.name
        ):
            if arch.enable:
                self.do_arch(arch, vars.copy(), makeflags.copy())

    def do_extra(self) -> None:
        try:
            packages_extra = self.templates.get_control("extra.control", self.vars)
        except KeyError:
            return

        extra_arches: dict[str, Any] = {}
        for package in packages_extra:
            arches = package['Architecture']
            for arch in arches:
                i = extra_arches.get(arch, [])
                i.append(package)
                extra_arches[arch] = i
        for arch in sorted(extra_arches.keys()):
            self.bundle.add_packages(packages_extra, (arch, ),
                                     MakeFlags(), check_packages=False)

    def do_indep_featureset(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        vars['localversion'] = ''
        if config.name_featureset != 'none':
            vars['localversion'] = '-' + config.name_featureset

        self.do_indep_featureset_setup(config, vars, makeflags)
        self.do_indep_featureset_makefile(config, vars, makeflags)
        self.do_indep_featureset_packages(config, vars, makeflags)

    def do_indep_featureset_setup(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_indep_featureset_makefile(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        makeflags['FEATURESET'] = config.name

    def do_indep_featureset_packages(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_arch(
        self,
        config: ConfigMergedDebianarch,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        vars['arch'] = config.name

        self.do_arch_setup(config, vars, makeflags)
        self.do_arch_makefile(config, vars, makeflags)
        self.do_arch_packages(config, vars, makeflags)
        self.do_arch_recurse(config, vars, makeflags)

    def do_arch_setup(
        self,
        config: ConfigMergedDebianarch,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_arch_makefile(
        self,
        config: ConfigMergedDebianarch,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        makeflags['ARCH'] = config.name

    def do_arch_packages(
        self,
        config: ConfigMergedDebianarch,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_arch_recurse(
        self,
        config: ConfigMergedDebianarch,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        for featureset in config.featuresets:
            if featureset.enable:
                self.do_featureset(featureset, vars.copy(), makeflags.copy())

    def do_featureset(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        vars['localversion'] = ''
        if config.name_featureset != 'none':
            vars['localversion'] = '-' + config.name_featureset

        self.do_featureset_setup(config, vars, makeflags)
        self.do_featureset_makefile(config, vars, makeflags)
        self.do_featureset_packages(config, vars, makeflags)
        self.do_featureset_recurse(config, vars, makeflags)

    def do_featureset_setup(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_featureset_makefile(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        makeflags['FEATURESET'] = config.name

    def do_featureset_packages(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def do_featureset_recurse(
        self,
        config: ConfigMergedFeatureset,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        for flavour in config.flavours:
            if flavour.enable:
                self.do_flavour(flavour, vars.copy(), makeflags.copy())

    def do_flavour(
        self,
        config: ConfigMergedFlavour,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        vars['localversion'] += '-' + config.name_flavour

        self.do_flavour_setup(config, vars, makeflags)
        self.do_flavour_makefile(config, vars, makeflags)
        self.do_flavour_packages(config, vars, makeflags)

    def do_flavour_setup(
        self,
        config: ConfigMergedFlavour,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        for i in (
            ('kernel-arch', 'KERNEL_ARCH'),
            ('localversion', 'LOCALVERSION'),
        ):
            if i[0] in vars:
                makeflags[i[1]] = vars[i[0]]

    def do_flavour_makefile(
        self,
        config: ConfigMergedFlavour,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        makeflags['FLAVOUR'] = config.name

    def do_flavour_packages(
        self,
        config: ConfigMergedFlavour,
        vars: dict[str, str],
        makeflags: MakeFlags,
    ) -> None:
        pass

    def substitute(self, s: str | list | tuple, vars) -> str | list:
        if isinstance(s, (list, tuple)):
            return [self.substitute(i, vars) for i in s]

        def subst(match) -> str:
            return vars[match.group(1)]

        return re.sub(r'@([-_a-z0-9]+)@', subst, str(s))

    def write(self) -> None:
        for bundle in self.bundles.values():
            bundle.extract_makefile()
            bundle.merge_build_depends()
            bundle.write()


def merge_packages(packages, new, arch) -> None:
    for new_package in new:
        name = new_package['Package']
        if name in packages:
            package = packages.get(name)
            package['Architecture'].add(arch)

            for field in ('Depends', 'Provides', 'Suggests', 'Recommends',
                          'Conflicts'):
                if field in new_package:
                    if field in package:
                        v = package[field]
                        v.extend(new_package[field])
                    else:
                        package[field] = new_package[field]

        else:
            new_package['Architecture'] = arch
            packages.append(new_package)
