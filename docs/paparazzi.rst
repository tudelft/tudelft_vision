
==============================
Getting started with Paparazzi
==============================

What is Paparazzi?
==================

Summarized from the `Paparazzi wiki <http://wiki.paparazziuav.org/>`__:

    '*Paparazzi is an open-source drone hardware and software project encompassing autopilot systems and ground station
    software for multicopters/multirotors, fixed-wing, helicopters and hybrid aircraft. Paparazzi enables
    users to add more features and improve the system. Using and improving Paparazzi is encouraged by the community.*'

Paparazzi is the recommended platform to implement computer vision behavior in drone flight. While the
Vision Library deals with the computer vision related hardware, Paparazzi is a complete airborne system and can be used
to (autonomously) operate almost any airframe. An example of such an airframe is the :doc:`Parrot Bebop </bebop>`.

Where to start
===============

New to Paparazzi? It can be quite overwhelming at first. This page covers the installation of paparazzi in Ubuntu
and will get you ready to start hacking. If you are not using Ubuntu, you can refer to the `wiki <http://wiki.paparazziuav.org/wiki/Installation>`__
for details on how to install Paparazzi on other platforms.

.. note::

    Are you familiar with Git version control? Paparazzi is updated `regularly <https://github.com/paparazzi/paparazzi/pulse>`__
    and merging your work with others can be complicated. A human-readable introduction to Git can be found
    `here <https://red-badger.com/blog/2016/11/29/gitgithub-in-plain-english>`__. The offical book is hosted `here <https://git-scm.com/book>`__.

Video tutorial
==============

The following video provides a step-by-step visual guide for installing and running the latest Paparazzi release.
It has been verified to work with Ubuntu 12.04 and up, including the latest 16.04 LTS release.

.. raw:: html

    <div style="position: relative; padding-bottom: 56.25%; height: 0; overflow: hidden; max-width: 100%; height: auto;">
        <iframe src="https://www.youtube.com/embed/eW0PCSjrP78"
            frameborder="0"
            allowfullscreen
            style="position: absolute; top: 0; left: 0; width: 100%; height: 100%;">
        </iframe>
    </div>

Ubuntu installation
===================

Add the installation sources for the software packages from a terminal and install the main Paparazzi development package::

    sudo add-apt-repository ppa:paparazzi-uav/ppa
    sudo apt-get update
    sudo apt-get install paparazzi-dev

Install the ARM cross compiling toolchain to enable compiling for common UAV hardware::

    sudo add-apt-repository ppa:team-gcc-arm-embedded/ppa
    sudo apt-get update
    sudo apt-get install gcc-arm-embedded

Navigate to your workspace and clone the git repository (intending to contribute? try `ssh <https://help.github.com/articles/adding-a-new-ssh-key-to-your-github-account/>`__)::

    git clone https://github.com/paparazzi/paparazzi

Change directory to Paparazzi and checkout the `latest release <https://github.com/paparazzi/paparazzi/releases/latest>`__ and compile the software::

    cd paparazzi
    git checkout v5.10
    make

You can now launch the Paparazzi Center::

    ./paparazzi

