#!/bin/bash

# 按需选择是否需要静态编译
gcc fuc_hp_app.c -o dpu_hotplug
# gcc -static gcc fuc_hp_app.c -o dpu_hotplug
