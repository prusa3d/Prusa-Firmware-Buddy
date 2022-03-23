# Resources for the PrusaLink web server

We have assortment of static "files" served by the PrusaLink web server. This
acts as a client-side application and offloads as much processing there.

The application comes from the <https://github.com/prusa3d/Prusa-Link-Web>.
Updating:

* Clone the above repository.
* Build the `mini` configuration.
* Replace the content of this directory (keep the favicon and the README).
* Update the rules in `CMakeList.txt` one level up.
* The build system will automatically gzip-compress them and embed the results
  in the output.
