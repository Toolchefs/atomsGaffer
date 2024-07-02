# AtomsGaffer

The Tool Chefs have open-sourced their [Atoms Crowd](https://atoms.toolchefs.com) extension for [Gaffer](http://www.gafferhq.org).

> Note : If you are an existing Tool Chefs customer, please contact Toolchefs at support@toolchefs.com to have a compiled version of AtomsGaffer.

> Caution : This project is still a non-functional work in progress.

### Build Instructions

**Requires:**

* cmake
* Gaffer Install
* AtomsMaya Install

**In a terminal:**

```
cd atomsGaffer
cmake -DGAFFER_ROOT=/opt/gaffer-1.4.5.0-linux-gcc9 -DATOMS_MAYA_ROOT=/opt/Toolchefs/AtomsMaya -DMAYA_MAJOR_VERSION=2023 -DPYTHON_VERSION=3.10 -DCMAKE_INSTALL_PREFIX=/opt/Toolchefs/AtomsGaffer -DOPENEXR_MAJOR_VERSION=3 -DOpenEXR_VERSION_MAJOR=1 -DIMATH_ROOT=/opt/gaffer-1.4.5.0-linux-gcc9 .
make install -j <num cores>)
```

### Runtime Instructions

Now that you've installed the extension, you need to tell Gaffer about it:

```
export toolchefs_LICENSE=5053@localhost
export ATOMS_GAFFER_ROOT=/opt/Toolchefs/AtomsGaffer
export GAFFER_EXTENSION_PATHS=${ATOMS_GAFFER_ROOT}:${GAFFER_EXTENSION_PATHS}
export LD_LIBRARY_PATH=/opt/Toolchefs/AtomsMaya/lib/2023:${LD_LIBRARY_PATH}
gaffer
```

Next, test your install:

`gaffer test AtomsGafferTest AtomsGafferUITest`

Now run the `gaffer` gui as normal.
