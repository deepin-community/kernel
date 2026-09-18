#!/usr/bin/python3

import sys
import glob
import os

from debian_linux.debian import Changelog, VersionLinux


class CheckImage(object):
    def __init__(self, config, dir, arch, featureset, flavour):
        self.dir = dir
        self.arch, self.featureset, self.flavour = arch, featureset, flavour

        self.changelog = Changelog(version=VersionLinux)[0]

        self.config_entry_base = config.merge('base', arch, featureset,
                                              flavour)
        self.config_entry_build = config.merge('build', arch, featureset,
                                               flavour)
        self.config_entry_image = config.merge('image', arch, featureset,
                                               flavour)

    def __call__(self, out):
        image = self.config_entry_build.get('image-file')
        uncompressed_image = self.config_entry_build \
                                 .get('uncompressed-image-file')

        if not image:
            # TODO: Bail out
            return 0

        image = os.path.join(self.dir, image)
        if uncompressed_image:
            uncompressed_image = os.path.join(self.dir, uncompressed_image)

        fail = 0

        fail |= self.check_size(out, image, uncompressed_image)

        return fail

    def check_size(self, out, image, uncompressed_image):
        value = self.config_entry_image.get('check-size')

        if not value:
            return 0

        dtb_size = 0
        if self.config_entry_image.get('check-size-with-dtb'):
            for dtb in glob.glob(
                    os.path.join(self.dir, 'arch',
                                 self.config_entry_base['kernel-arch'],
                                 'boot/dts/*.dtb')):
                dtb_size = max(dtb_size, os.stat(dtb).st_size)

        size = os.stat(image).st_size + dtb_size

        # 1% overhead is desirable in order to cope with growth
        # through the lifetime of a stable release. Warn if this is
        # not the case.
        usage = (float(size)/value) * 100.0
        out.write('Image size %d/%d, using %.2f%%.  ' % (size, value, usage))
        if size > value:
            out.write('Too large.  Refusing to continue.\n')
            return 1
        elif usage >= 99.0:
            out.write('Under 1%% space in %s.  ' % self.changelog.distribution)
        else:
            out.write('Image fits.  ')
        out.write('Continuing.\n')

        # Also check the uncompressed image
        if uncompressed_image and \
           self.config_entry_image.get('check-uncompressed-size'):
            value = self.config_entry_image.get('check-uncompressed-size')
            size = os.stat(uncompressed_image).st_size
            usage = (float(size)/value) * 100.0
            out.write('Uncompressed Image size %d/%d, using %.2f%%.  ' %
                      (size, value, usage))
            if size > value:
                out.write('Too large.  Refusing to continue.\n')
                return 1
            elif usage >= 99.0:
                out.write('Uncompressed Image Under 1%% space in %s.  ' %
                          self.changelog.distribution)
            else:
                out.write('Uncompressed Image fits.  ')
            out.write('Continuing.\n')

        return 0


class Main(object):
    def __init__(self, dir, arch, featureset, flavour):
        self.args = dir, arch, featureset, flavour

        # TODO
        # self.config = ConfigCoreDump(open("debian/config.defines.dump", "rb"))

    def __call__(self):
        fail = 0

        for c in ():
            fail |= c(self.config, *self.args)(sys.stdout)

        return fail


if __name__ == '__main__':
    sys.exit(Main(*sys.argv[1:])())
