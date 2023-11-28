all: meerkat-deb-package

meerkat-deb-package:
	cd $(BINARY_DIR) && { \
		rm -rf debian; \
		rm -rf boson-meerkat.deb; \
	}

	cd $(BINARY_DIR) && { \
		mkdir -p debian/usr/bin; \
		cp ${DIST_DIR}/bin/boson-launcher debian/usr/bin; \
		strip debian/usr/bin/boson-launcher; \
		mkdir -p debian/etc/boson-meerkat; \
		cp ${DIST_DIR}/etc/boson-launcher/default.conf debian/etc/boson-meerkat; \
		mkdir -p debian/lib/systemd/system; \
		cp $(SOURCE_DIR)/debian/scripts/boson-meerkat.service debian/lib/systemd/system; \
		mkdir -p debian/etc/init.d; \
		cp $(SOURCE_DIR)/debian/scripts/boson-meerkat debian/etc/init.d/boson-meerkat; \
		mkdir -p debian/var/lib/boson-meerkat; \
		mkdir -p debian/var/lib/boson-meerkat/db; \
		mkdir -p debian/var/log/boson-meerkat; \
		mkdir -p debian/DEBIAN; \
		cp $(SOURCE_DIR)/debian/control/* debian/DEBIAN; \
		dpkg-deb --build debian boson-meerkat.deb; \
	}

