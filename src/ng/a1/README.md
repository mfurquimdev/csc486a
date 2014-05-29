3D Spline Editor
===============

This is a 3D spline editor for the first assignment of csc486a.

It was programmed using Xlib and OpenGL, therefore it currently only works on platforms which support the X11 window manager.

This application makes use of a multi-threaded multi-contexted renderer written using C++11. The renderer has two threads: The rendering thread, and the resource thread. The rendering thread is double-buffered and the buffers are swapped at the end of each frame of rendering. The resource thread uses a semaphore to try to load resources (such as shaders of buffers) as soon as OpenGL makes it possible (this allows streaming 3D worlds without pausing gameplay).

Operations on Control Points
----------------------------

* Create new control points by left clicking

* Select control points by left clicking on them

* Delete the currently selected control point by pressing the **Delete** key.

* Move control points on their X-Z plane by dragging and dropping them with the left mouse button.

* Move control points up and down their Y axis by scrolling the mouse wheel while holding your click on them with the left mouse button.

* Unselect the current node and hide the guidelines by pressing **Escape**.

* Alternate the mode of the node traveling along the spline using the **Tab** key. It alternates between three choices: Parametric, ConstantVelocity, and EaseInEaseOut.

Camera Movement
------------------

* Zoom the camera in and out by scrolling.

* Rotate the camera by holding right click and moving the mouse left or right.

Calculating Arc Length
---------------------

* The current arc length is always displayed in the title of the window.
