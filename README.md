Lightpad
=========

**A simple editor, based on keyboard shortcuts**

Lightpad is a simple text editor, based on a keyboard-based workflow: there are no graphical elements besides the tabs, the source view and the statusbar. This means that there are no buttons and menus and there never will be.

Lightpad tries to stay compatible with GEdit shortcut-wise. To help you get going, [here][article] is a list of standard GtkSourceView key combinations as well as some GEdit specific onces. Of course, keyboard shortcuts for features Lightpad does not have are not implemented (such as spell checking and printing).

[article]: http://hamwaves.com/gedit/en/index.html

**Disclaimer:** this README is written with Lightpad! ;)

Installation
------------

The only dependencies required to build and run Lightpad, are GTK3 and GtkSourceView.

Once the ridiculous amount of dependencies are installed, just run these commands to build and install Lightpad::

    $ make
    # make clean install

Bugs
----

For any bug or request [fill an issue][bug] on [GitHub][ghp].

  [bug]: https://github.com/Unia/lightpad/issues
  [ghp]: https://github.com/Unia/lightpad

License
-------
**Lightpad** is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

**Lightpad** is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <http://www.gnu.org/licenses/>.

**Copyright © 2014** Jente Hidskes <hjdskes@gmail.com>

Some code is based on/copied from the TutorialTextEditor. These parts are licensed under the GPLv2 license.
**Copyright © 2007** Micah Carrick <email@micahcarrick.com>

Another great help at things I could not figure out myself has been Olivier Brunel - thank you for your help!
