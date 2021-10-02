
.. _getting-started:

###############
Getting Started
###############
To start developing a VAP with the new API, please use the `create_adi_project <http://xarray.pydata.org/en/stable/>`_
tool to stub out the new file format::

        [lansing@emerald ~]$ create_adi_project
        usage: create_adi_project [-h] [-t TEMPLATE] [-p PROCESS] [-o OUTPUT]
                                  [-a AUTHOR] [-n PHONE] [-e EMAIL] [-i INPUT]
                                  [-d DUMP_JSON] [-v DOD_VERSION]

In particular, use one of the two new templates::

         ----------------------------------------
         New VAP TEMPLATES available in Py
         vap-py-retriever  --> uses the new xarray API
         vap-py-transform  --> uses the new xarray API
         ----------------------------------------



The main difference between old ADI Py processes and ones created with the new API is the ``__main__.py`` file.
Instead of implementing dsproc hook functions individually, new processes will extend the `Process <autoapi/adi_py/process/index.html>`_
base class and then the main function will simply call Process.run() to invoke ADI.

.. admonition:: Note

    Much of the functionality that was previously in the __main__.py template is now built into the Process base class
    (or XArrray).  Therefore, you will see that the new templates contain significantly less boilerplate code.

Each Process can implement any of the ADI hooks by overriding the default (empty) implementation in the base
class.  Please see the `tutorial_py <https://code.arm.gov/vap/tutorial_py>`_ project for an example.  Also
review the `Process API Reference <autoapi/adi_py/process/index.html>`_ for a list of functions that are
available to your class.


.. admonition:: Prerequisites

    For the best experience working with the new ADI Python API,
    we recommend a solid foundation in the XArray and Numpy
    libraries.  Below are a list of tutorials to get you
    started:

    1. `<https://xarray-contrib.github.io/xarray-tutorial/scipy-tutorial/00_overview.html>`_
    2. `<https://www.youtube.com/watch?v=mecN-Ph_-78>`_
    3. `<https://www.youtube.com/watch?v=xdrcMi_FB8Q>`_
    4. `<http://gallery.pangeo.io/repos/pangeo-data/pangeo-tutorial-gallery/xarray.html>`_



Developing with an IDE
^^^^^^^^^^^^^^^^^^^^^^
We **strongly** recommend that you use an IDE such as VSCode or PyCharm to develop your code remotely.  This will
enable you to get autocomplete support for all Process functions which include built-in Python docstrings.  This
will make it significantly easier to work with the new API.  In addition, you will be able to set break points and
step through your code which will make it much easier to debug.

The new create_adi_project templates will provide a **.vscode folder** that contains a README file with instructions
and all the config files needed to get started debugging remotely with VSCode.  Additional instructions for PyCharm
will be provided at a later time.
`Click here to view the VSCode README file. <https://code.arm.gov/adi/create_adi_project/-/blob/master/create_adi_project/data/vap-py-vscode/.vscode/README.md>`_







