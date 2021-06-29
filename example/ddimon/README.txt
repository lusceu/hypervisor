# Build instructions

1. Build example_ddimon:

```
cmake "..\hypervisor" -DHYPERVISOR_EXTENSIONS=example_ddimon -DHYPERVISOR_EXTENSIONS_DIR="$PWD\..\hypervisor\example\ddimon\" -DFETCHCONTENT_SOURCE_DIR_BSL="$PWD\..\bsl\"
ninja
ninja driver_build

```

2. Run DdiMon example:

```
# start hypervisor extension
ninja driver_load
ninja start

# run driver providing the source addresses of the target functions
ddimon_hookidnt_load

# get output
ninja dump

# stop hypervisor extension
ninja stop

```
