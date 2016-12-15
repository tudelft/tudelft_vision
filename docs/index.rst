.. toctree::
   :maxdepth: 2
   :hidden:

   self
   paparazzi
   bebop

=======================
TU Delft Vision Library
=======================

What is the Vision Library?
===========================

The TU Delft Vision Library allows computer vision developers to have a single codebase for the vision programs
and deploy on multiple platforms with ease. Possible usages of the library could be in the field of autonomous
UAV development. The library handles the communication with the camera as well as encoding and networking of the
video frames.


Features
========

* Support for Linux and the :doc:`Parrot Bebop </bebop>` platform
* JPEG and H264 encoding
* Streaming via Real-time Transport Protocol (RTP)


Application
============

An example of utilizing the Vision Library is shown in the next video. The Parrot Bebop drone runs an autopilot system
comprised of the Vision Library and `Paparazzi UAV <http://wiki.paparazziuav.org/>`_.


.. raw:: html

    <div style="position: relative; padding-bottom: 56.25%; height: 0; overflow: hidden; max-width: 100%; height: auto;">
        <iframe src="https://www.youtube.com/embed/y7svb4qPvT8"
            frameborder="0"
            allowfullscreen
            style="position: absolute; top: 0; left: 0; width: 100%; height: 100%;">
        </iframe>
    </div>