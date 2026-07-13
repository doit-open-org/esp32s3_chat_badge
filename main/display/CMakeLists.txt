# 递归收集当前目录下的 .c/.cpp 源码
file(GLOB_RECURSE UI_SRC CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/page/*/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

# 组织需要导出的包含路径
set(UI_INC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/page_manager/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/page/**
    ${CMAKE_CURRENT_SOURCE_DIR}/component
)

# 把源码与包含路径“导出”给上层（不生成任何目标/库）
set(AI_S2_SOURCES ${UI_SRC} PARENT_SCOPE)
set(AI_S2_INCLUDE_DIRS ${UI_INC} PARENT_SCOPE)

# 如果需要给全局加一些编译宏（例如启用简化 lv_conf 包含），可按需导出变量或在顶层添加：
# set(AI_S2_DEFS LV_CONF_INCLUDE_SIMPLE PARENT_SCOPE)
