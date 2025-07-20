# Dependencies folder links to external folders on the system
# Enables specifying exact paths for inclusion within cmake, without having
# 'additional installations' interfering with the desired use
#
# e.g. SDL 1.2 installed within the distribution, but targeting 2.0
# !!! TODO: this loses function with using pkg-config, as it returns raw paths !!!
