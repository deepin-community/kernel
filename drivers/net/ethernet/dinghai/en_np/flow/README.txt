一、脚本说明
1、flow目录下有两个xml文件，zxdh_flow_attr_api和zxdh_flow_attr_demo，zxdh_flow_attr_api.xml编写请参照demo；
2、demo只作为示例使用，不参与编译；api作为编译脚本，用于生成api/source和api/include中的文件，对外提供api接口；
二、编译脚本使用说明
1、按照正确流表资源填写zxdh_flow_attr_api.xml，注意比特位要求连续且不能重叠；
2、在en_np/flow目录下执行python3 tool.py api，分别生成en_np/flow/api下的文件；
三、补充说明

注意：请确保python的解释器版本大于等于3.80