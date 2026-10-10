# /usr/bin/env python
# -*- coding: utf-8 -*-
import sys
import re
import string
import xlrd
import xlwt
import sys, os.path
import time
from collections import defaultdict

file_out_path = './c_code/'

fp_dpp_drv_eram_c = open(file_out_path + 'dpp_drv_eram.c', 'w')
fp_dpp_drv_eram_c.truncate()

print('#include "dpp_drv_eram.h"', file=fp_dpp_drv_eram_c)
print('', file=fp_dpp_drv_eram_c)

fp_dpp_drv_eram_h = open(file_out_path + 'dpp_drv_eram.h', 'w')
fp_dpp_drv_eram_h.truncate()

print('\n#ifndef DPP_DRV_ERAM_H', file=fp_dpp_drv_eram_h)
print('#define DPP_DRV_ERAM_H', file=fp_dpp_drv_eram_h)
print('', file=fp_dpp_drv_eram_h)
print('#include "zxic_common.h"', file=fp_dpp_drv_eram_h)
print('', file=fp_dpp_drv_eram_h)

def gen_eram_info_c(fp_excel_flie, table_name):

    for i in range(0, len(fp_excel_flie.sheets())):
        sheet = fp_excel_flie.sheet_by_index(i)
        if (sheet.name != table_name):
            continue

        table_name = sheet.cell_value(1, 0)
        table_type = sheet.cell_value(1, 1)
        table_width = sheet.cell_value(1, 2)
        field_bits = sheet.col_values(4)
        field_name = sheet.col_values(5)

        if (table_type != '直接表'):
            continue

        print('typedef struct zxdh_' + table_name.lower() + '_t', file=fp_dpp_drv_eram_h)
        print('{', file=fp_dpp_drv_eram_h)
        for value in field_name[1:][::-1]:
            if (value.lower() == 'rsv'):
                continue 
            print('    ZXIC_UINT32 ' + value.lower() + ';', file=fp_dpp_drv_eram_h)
        print('} ZXDH_' + table_name.upper() + '_T;', file=fp_dpp_drv_eram_h)

        print('ZXIC_UINT32 dpp_apt_set_' + table_name.lower() + '_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4])',\
                                                                                            file=fp_dpp_drv_eram_c)
        print('{', file=fp_dpp_drv_eram_c)
        print('    ZXDH_' + table_name.upper() + '_T *attr = (ZXDH_' + table_name.upper() + '_T *)pData;', file=fp_dpp_drv_eram_c)
        print('', file=fp_dpp_drv_eram_c)

        for bits, name in zip(field_bits[1:], field_name[1:]):
            if (name.lower() == 'rsv'):
                continue

            start_bit = int(bits.split(':')[0])
            end_bit = int(bits.split(':')[1])

            bit_width = start_bit - end_bit + 1

            byte_index_by_start_bit = 3 - (start_bit // 32)
            byte_index_end_bit = 3 - (end_bit // 32)

            start_bit_in_byte = (start_bit % 32) - bit_width + 1

            if (byte_index_by_start_bit == byte_index_end_bit):
                print(f'    ZXIC_COMM_UINT32_WRITE_BITS(buff[{byte_index_by_start_bit}], attr->{name.lower()}, {start_bit_in_byte}, {bit_width});', file=fp_dpp_drv_eram_c)
            else:
                print(f'    ZXIC_COMM_UINT32_WRITE_BITS(buff[{byte_index_by_start_bit}], attr->{name.lower()} >> {bit_width - (start_bit % 32 + 1)}, {0}, {(start_bit % 32 + 1)});', file=fp_dpp_drv_eram_c)
                print(f'    ZXIC_COMM_UINT32_WRITE_BITS(buff[{byte_index_end_bit}], attr->{name.lower()}, {end_bit % 32}, {32 - start_bit_in_byte % 32});', file=fp_dpp_drv_eram_c)

        print('', file=fp_dpp_drv_eram_c)
        print('    return DPP_OK;', file=fp_dpp_drv_eram_c)
        print('}', file=fp_dpp_drv_eram_c)
        print('', file=fp_dpp_drv_eram_c)


        print('ZXIC_UINT32 dpp_apt_get_' + table_name.lower() + '_data(ZXIC_VOID *pData, ZXIC_UINT32 buff[4])',\
                                                                                            file=fp_dpp_drv_eram_c)
        print('{', file=fp_dpp_drv_eram_c)
        print('    ZXDH_' + table_name.upper() + '_T *attr = (ZXDH_' + table_name.upper() + '_T *)pData;', file=fp_dpp_drv_eram_c)
        print('', file=fp_dpp_drv_eram_c)

        for bits, name in zip(field_bits[1:], field_name[1:]):
            if (name.lower() == 'rsv'):
                continue

            start_bit = int(bits.split(':')[0])
            end_bit = int(bits.split(':')[1])

            bit_width = start_bit - end_bit + 1

            byte_index_by_start_bit = 3 - (start_bit // 32)
            byte_index_end_bit = 3 - (end_bit // 32)

            start_bit_in_byte = (start_bit % 32) - bit_width + 1

            if (byte_index_by_start_bit == byte_index_end_bit):
                print(f'    ZXIC_COMM_UINT32_GET_BITS(attr->{name.lower()}, buff[{byte_index_by_start_bit}], {start_bit_in_byte}, {bit_width});', file=fp_dpp_drv_eram_c)
            else:
                print(f'    ZXIC_COMM_UINT32_GET_BITS(attr->{name.lower()} >> {bit_width - (start_bit % 32 + 1)}, buff[{byte_index_by_start_bit}], {0}, {(start_bit % 32 + 1)});', file=fp_dpp_drv_eram_c)
                print(f'    ZXIC_COMM_UINT32_GET_BITS(attr->{name.lower()}, buff[{byte_index_end_bit}], {end_bit % 32}, {32 - start_bit_in_byte % 32});', file=fp_dpp_drv_eram_c)

        print('', file=fp_dpp_drv_eram_c)
        print('    return DPP_OK;', file=fp_dpp_drv_eram_c)
        print('}', file=fp_dpp_drv_eram_c)


file_name  = "标卡表项定义说明.xlsx"
table_name = sys.argv[1]

# 脚本入口
print("%s 数据结构及码流转换接口编码中..."% table_name)

if not os.path.isfile(file_name):
    print('Error: %s 文件不存在' % (file_name))
    sys.exit()

fp_excel_flie = xlrd.open_workbook(file_name)
gen_eram_info_c(fp_excel_flie, table_name)

print('', file=fp_dpp_drv_eram_h)
print('#endif', file=fp_dpp_drv_eram_h)


fp_dpp_drv_eram_c.close()
fp_dpp_drv_eram_h.close()

print("%s 数据结构及码流转换接口编码完成!"% table_name)
