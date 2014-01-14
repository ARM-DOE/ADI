ADI
===

ARM Data Integrator, ADI, is an open source framework that automates the process of retrieving and preparing data for analysis, simplifies the design and creation of output data products produced by the analysis, and provides a modular, flexible software development architecture for implementing algorithms.  These capabilities are supported through the use of a workflow for data integration, a source code generator that produces C, IDL and Python templates, and a graphical interface through which users can efficient define their data input, preprocessing, and output characteristics.  

ADI is used by the 'Atmospheric Radiation Measurement (ARM) Climate Research Facility <http://www.arm.gov>' to process raw data collected from ARM instruments, and to implement scientific algorithms that process one or more of ARM's existing data products to produce new, higher value, data products.

Important Links
===============

- Official source code repository: https://github.com/ARM-DOE/ADI
- HTML documentation: http://engineering.arm.gov/ADI_doc
- ADI Process Configuration Interface https://engineering.arm.gov/pcm/Main.html
- Instructions for installing a rh6 build https://engineering.arm.gov/~gaustad/instructions.html
- Examples: http://engineering.arm.gov/ADI_doc/algorithm.html#algorithm-development-tutorial
- ARM Data Archive for accessing ARM data http://archive.arm.gov

Dependencies
============

ADI has been tested to run under Red Hat Enterprise Linux 2.5 and 2.6.  Development in Python is for 2.7+, but does not include Python 3.  Development in IDL requires IDL 8.2+.  

To install a rhel6 build, the only other dependency not provided as part of the install of the build is:
* 'epl6 <https://fedoraproject.org/wiki/EPEL>'


The source code is freely availaable, but instructions and Makefiles to support compilation are not.  ARM's development environment uses a in house workflow management system to streamline development activities.  Generic Makefiles are not currently available, but will be provided at a later date. 

The required dependencies to install and compile source code are:
* 'Unidata NetCDF4-C <http://www.unidata.ucar.edu/downloads/netcdf>'
* 'PostgreSQL 8.4 <ttp://www.postgresql.org/download/linux/redhat/>'



Optional Dependences
====================

* 'Multiprotocol file transform library <http://curl.haxx.se/libcurl/>'_7.19.7 is used to access DSDB via a Web Service.

To develop new source code templates beyond those provided:
* 'Cheeta Pthon Template Engine <http://www.cheetahtemplate.org/>'

To develop algorithms in Python 2.7+:
* 'netCDF4 <http://code.google.com/p/netcdf4-python/>'_ 1.0.2+ 
* 'NumPy <http://www.scipy.org>'_ 1.6+

Install
=======

Installation and complilation of the source code is not fully documented, but will be in the future.  
However the latest source code is available with the command::
 
    git clone https://github.com/ARM-DOE/ADI.git


To install a build please  follow the directions at https://engineering.arm.gov/~gaustad/instructions.html

