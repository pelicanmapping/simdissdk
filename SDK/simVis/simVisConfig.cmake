include(CMakeFindDependencyMacro)
find_dependency(simNotify)
find_dependency(simCore)
find_dependency(simData)
include("${CMAKE_CURRENT_LIST_DIR}/simVisTargets.cmake")