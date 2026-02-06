#!/bin/bash

set -euo pipefail

REMOTE_NAME="${REMOTE_NAME:-torvalds}"
REMOTE_URL="${REMOTE_URL:-https://github.com/torvalds/linux.git}"
TARGET_BRANCH="${2:-master}"

log_info() { echo -e "\033[32m[INFO]\033[0m $*" >&2; }
log_error() { echo -e "\033[31m[ERROR]\033[0m $*" >&2; }

main() {
    [ $# -lt 1 ] && { echo "Usage: $0 <range> [branch]" >&2; exit 1; }

    local range="$1"

    if ! git remote get-url torvalds >/dev/null 2>&1; then
        git remote add torvalds "${REMOTE_URL}"
    else
        git remote set-url torvalds "${REMOTE_URL}"
    fi

    git fetch torvalds

    if ! git rev-parse --verify "$TARGET_BRANCH^{commit}" >/dev/null 2>&1; then
        log_error "目标分支不存在: $TARGET_BRANCH"
        exit 1
    fi

    local tmp_target=$(mktemp)

    log_info "正在导出 $TARGET_BRANCH 标题..."
    git log --format="%s" "$TARGET_BRANCH" 2>/dev/null > "$tmp_target"
    local target_count=$(wc -l < "$tmp_target")
    log_info "目标分支共 $target_count 个标题"

    local total=$(git rev-list --count "$range" 2>/dev/null || echo "0")
    log_info "检查范围 $range (共 $total 个提交)..."

    log_info "开始筛选..."
    echo "========================================"

    # 使用标准AWK（兼容mawk/gawk），避免match()数组捕获
    git log --format="%H%n%s%n%b%n---COMMIT_END---" --reverse "$range" 2>/dev/null | \
    awk -v target_file="$tmp_target" -v total="$total" '
    BEGIN {
        # 加载目标分支标题到数组
        while ((getline line < target_file) > 0) {
            if (length(line) > 0) target[line] = 1
        }
        close(target_file)

        matched = 0
        dup = 0
        no_fixes = 0
        no_ref = 0
        processed = 0

        # 读取状态：0=读hash, 1=读subject, 2=读body
        state = 0
    }

     /---COMMIT_END---/ {
        # 处理完一个commit
        if (hash != "") {
            processed++

            # 条件1：标题是否已存在？
            if (subject in target) {
                dup++
            } else {
                # 条件2：查找 Fixes: 行
                has_fixes = 0
                ref_found = 0

                # 分割body行查找 Fixes:
                n = split(body, lines, /\n/)
                for (i=1; i<=n; i++) {
                    if (lines[i] ~ /^Fixes:/) {
                        has_fixes = 1

                        # 提取 ("...") 中的内容（兼容所有awk版本）
                        line_content = lines[i]
                        start_pos = index(line_content, "(\"") 

                        if (start_pos > 0) {
                            # 找到 (" 的位置，提取到 ") 结束
                            temp = substr(line_content, start_pos + 2)
                            end_pos = index(temp, "\")")

                            if (end_pos > 0) {
                                ref_title = substr(temp, 1, end_pos - 1)

                                # 检查引用是否在目标分支
                                if (ref_title in target) {
                                    ref_found = 1
                                    break
                                }
                            }
                        }
                    }
                }

                if (!has_fixes) {
                    no_fixes++
                } else if (!ref_found) {
                    no_ref++
                } else {
                    # 符合条件：有Fixes且引用存在，且标题不重复
                    matched++
                    print hash " " subject
                }
            }
        }

        state = 0
        hash = ""
        subject = ""
        body = ""
        next
    }

    state == 0 {
        hash = $0
        state = 1
        next
    }

    state == 1 {
        subject = $0
        state = 2
        next
    }

    state == 2 {
        # 累积body行
        if (body == "") body = $0
        else body = body "\n" $0
        next
    }

    END {
        printf "\r\033[K" > "/dev/stderr"
        print "[INFO] 完成: 总计 " processed " | 匹配 " matched " | 重复 " dup " | 无Fixes " no_fixes " | 缺依赖 " no_ref > "/dev/stderr"
    }
    '

    echo "========================================" >&2

    rm -f "$tmp_target"
}

main "$@"
