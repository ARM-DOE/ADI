# ADI

ARM Data Integrator, ADI, is an open source framework that automates the process of retrieving and preparing data for analysis, simplifies the design and creation of output data products produced by the analysis, and provides a modular, flexible software development architecture for implementing algorithms.  These capabilities are supported through the use of a workflow for data integration, a source code generator that produces C, IDL and Python templates, and a graphical interface through which users can efficient define their data input, preprocessing, and output characteristics.  

ADI is used by the 'Atmospheric Radiation Measurement (ARM) Climate Research Facility <http://www.arm.gov>' to process raw data collected from ARM instruments, and to implement scientific algorithms that process one or more of ARM's existing data products to produce new, higher value, data products.

This package of the code is for users local work station (i.e. not ARM processing systems).  As such it is updated for major changes in functionality and not for minor incremental improvements.

---
## Building from source code
 
### Prerequisites

For MacOS users [Homebrew](https://docs.brew.sh/Installation) can be used to install the following dependencies. Linux users will need to install both the runtime and devel packages. The highest version available for all packages should work.

* **build system**
    * autoconf
    * automake
    * gcc
    * libtool
    * m4
    * pkgconfig

* **first order library dependencies**
    * netcdf
    * openssl
    * postgresql
    * udunits  (macOS - Homebrew)
    * udunits2 (Linux)
    * openblas (macOS - Homebrew)
    * atlas    (Linux)

* **optional** (required for python bindings)
    * python
    * cython (on macOS use 'pip3 install cython' instead of Homebrew)
    * numpy  (on macOS use 'pip3 install numpy'  instead of Homebrew)

* **optional**
    * sqlite

### Install

The following commands will download and install all ADI packages to /usr/local. You can change the value of the --prefix option to install to a different location. For more details run `./install_adi.sh -h`.

    git clone --depth 1 https://github.com/ARM-DOE/ADI.git
    cd ADI
    ./install_adi.sh --prefix=/usr/local

You will then need to add one of the following lines to your shell initialization file. Change /usr/local to the installation directory if you specified a different location. If your login shell is bash add this to your ~/.bash_profile and/or ~/.bashrc file.

    source /usr/local/etc/.adi.bashrc

If your login shell is csh add this line to your .cshrc file:

    source /usr/local/etc/.adi.cshrc

---
## Alternative Installation: Run on Host rh6 Machine (this needs to be updated for rh7)

### Dependencies

ADI has been tested to run under Red Hat Enterprise Linux 2.5 and 2.6.  Development in Python is for 2.7+, but does not include Python 3.  Development in IDL requires IDL 8.2+.  

To install a rhel6 build, the only other dependency not provided as part of the install of the build is:
* 'epl6 <https://fedoraproject.org/wiki/EPEL>'

Required packages provided in epel6 include:
- netcdf-4
- udunits2

### Optional Dependences

* 'Multiprotocol file transform library <http://curl.haxx.se/libcurl/>'_7.19.7 is used to access DSDB via a Web Service.

To develop new source code templates beyond those provided:
* 'Cheeta Pthon Template Engine <http://www.cheetahtemplate.org/>'

To develop algorithms in Python 2.7+:
* 'netCDF4 <http://code.google.com/p/netcdf4-python/>'_ 1.0.2+ 
* 'NumPy <http://www.scipy.org>'_ 1.6+

### To Install a Build of ADI:

- Ensure your version of red hat 6 is up to date
- Install epel 6, which contains packages ADI depends on. You can download the rpm (for 64-bit RH6) from  [here](http://dl.fedoraproject.org/pub/epel/6/x86_64/epel-release-6-8.noarch.rpm)
- Open the .rpm file with Red Hat's Package Installer and follow the prompts to install it.
- Create a file ARM.repo in /etc/yum.repos.d/ARM.repo with the following entries:
    - `[ARMP6]` 
    - `name=ARM Production RH6 Repository`
    - `baseurl=http://yum.arm.gov/prod6`
    - `gpgcheck=0`
- run the following commands:
    - `$> yum -y groupinstall adi6`
    - `$> yum install netcdf-devel`
- if developing in IDL, idl82 is required.
- if developing in Python, Python 2.7 is required. Python 3 is not supported

