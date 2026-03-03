Learn Embedded RTOS in 20 Days Documentation
=============================================

Welcome to the documentation hub for the Learn Embedded RTOS in 20 Days project.

.. toctree::
   :maxdepth: 2
   :caption: Contents

   days/index
   overview/README
   patterns/README
   comparison/README
   cheatsheets/README

Quick Start
-----------

1. Create a virtual environment.
2. Install docs dependencies.
3. Build HTML docs.

.. code-block:: bash

   python3 -m venv .venv
   source .venv/bin/activate
   pip install -r docs/requirements.txt
   sphinx-build -b html docs docs/_build/html
