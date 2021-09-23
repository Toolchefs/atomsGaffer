# AtomsGaffer

The Tool Chefs have open-sourced their [Atoms Crowd](https://atoms.toolchefs.com) extension for [Gaffer](http://www.gafferhq.org).

> Note : If you are an existing Tool Chefs customer, please contact Toolchefs at support@toolchefs.com to have a compiled version of AtomsGaffer.

> Caution : This project is still a non-functional work in progress.

### Build Instructions

**Requires:**

* cmake
* Gaffer Install
* Atoms Install

**In a terminal:**

```
setenv GAFFER_ROOT <gaffer install path>
setenv ATOMS_ROOT <atoms install path>
setenv ATOMSGAFFER_INSTALL_PREFIX <your desired install path>

cd atomsGaffer
cmake -DGAFFER_ROOT=$GAFFER_ROOT -DATOMS_ROOT=$ATOMS_ROOT -DCMAKE_CXX_FLAGS='-std=c++14' -DCMAKE_INSTALL_PREFIX=$ATOMSGAFFER_INSTALL_PREFIX .
make install -j <num cores>)
```

### Runtime Instructions

Now that you've installed the extension to `$ATOMSGAFFER_INSTALL_PREFIX`, you need to tell Gaffer about it:

`setenv GAFFER_EXTENSION_PATHS $ATOMSGAFFER_INSTALL_PREFIX:$GAFFER_EXTENSION_PATHS`

Next, test your install:

`gaffer test AtomsGafferTest AtomsGafferUITest`

Now run the `gaffer` gui as normal.
