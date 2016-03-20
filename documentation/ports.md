# Port installation script
The script `/patches/ports/port.sh` allows installing Ghost
ports with ease.

To for example install version 2.5.3 of freetype, run:

	sh port.sh freetype/2.5.3
	
This first looks for the version folder within the `freetype` folder
in `/patches/ports`. Each version folder must contain a `package.sh`
that contains the package configuration.


## Package installation
The version folder must contain a `package.sh` declaring the following:

* `REMOTE_ARCHIVE` specifies the remote location of the archive
* `UNPACKED_DIR` declaring where to unpack the tarball
* `ARCHIVE` declaring the name for downloading the archive
* `port_unpack` a task that unpacks the archive
* `port_install` a task that performs installation

When running the installation,

* `REMOTE_ARCHIVE` is downloaded to the folder
  `/patches/ports/build/<package>/<version>`
* downloaded archive is named to `ARCHIVE`
* `port_unpack` is executed
* when supplied, the `patch.diff` in the version folder is applied
* `port_install` target is executed