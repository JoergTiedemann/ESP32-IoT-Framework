from platformio.builder.tools.piolib import ProjectAsLibBuilder, PackageItem, LibBuilderBase
from platformio.package.version import get_original_version
from SCons.Script import ARGUMENTS  
from shlex import quote
Import("env", "projenv")


# from https://github.com/platformio/platformio-core/blob/develop/platformio/builder/tools/piolib.py
def _correct_found_libs(lib_builders):
    # build full dependency graph
    found_lbs = [lb for lb in lib_builders if lb.dependent]
    for lb in lib_builders:
        if lb in found_lbs:
            lb.search_deps_recursive(lb.get_search_files())
    for lb in lib_builders:
        for deplb in lb.depbuilders[:]:
            if deplb not in found_lbs:
                lb.depbuilders.remove(deplb)

print("Script entry point")

project = ProjectAsLibBuilder(env, "$PROJECT_DIR")

# rescan dependencies just like in py file above. otherwise dependenceis are empty
ldf_mode = LibBuilderBase.lib_ldf_mode.fget(project)
lib_builders = env.GetLibBuilders()
project.search_deps_recursive()
if ldf_mode.startswith("chain") and project.depbuilders:
    _correct_found_libs(lib_builders)

# for debugging
def _print_deps_tree(root, level=0):
    margin = "|   " * (level)
    for lb in root.depbuilders:
        title = "<%s>" % lb.name
        pkg = PackageItem(lb.path)
        if pkg.metadata:
            print("Packageowner:",pkg.metadata.spec.owner)
            title += " %s" % pkg.metadata.version
        elif lb.version:
            title += " %s" % lb.version
        print("%s|-- %s" % (margin, title), end="")
        if int(ARGUMENTS.get("PIOVERBOSE", 0)):
            if pkg.metadata and pkg.metadata.spec.external:
                print(" [%s]" % pkg.metadata.spec.url, end="")
            print(" (", end="")
            print(lb.path, end="")
            print(")", end="")
        print("")
        if lb.depbuilders:
            _print_deps_tree(lb, level + 1)

# create a map of all used libraries and their version.
# the structure of the tree is not captured, just library names and versions. 
library_versions = dict()
library_owner = dict()
def get_all_library_dependencies(root, level=0):
    global library_versions
    for lb in root.depbuilders:
        pkg = PackageItem(lb.path)
#       wir merken un s den den owner in einem separaten dictionary
        if pkg.metadata and pkg.metadata.spec and pkg.metadata.spec.owner:
            lib_owner = pkg.metadata.spec.owner
        else:
            lib_owner = ""
        lib_name = lb.name
#        print("Libname:",lib_name," Owner:",lib_owner)
        lib_version = pkg.metadata.version if pkg.metadata else lb.version
        library_versions[str(lib_name)] = str(lib_version)
        library_owner[str(lib_name)] = str(lib_owner)
        if lb.depbuilders:
            get_all_library_dependencies(lb, level + 1)

#print("PRINTING DEP TREE")
#_print_deps_tree(project)

get_all_library_dependencies(project)
#print(library_versions)

# convert found library names and versions into macros for code.
# style and formating can be arbitrary: here I chose to hold everything in one big string.
macro_value = ""
for lib, version in library_versions.items(): 
    strowner = library_owner[lib]
    #print(lib + ": " + version)
    # primitive: simply 'library':'version' format. does not escape anything specifically.
    # this is chosen to work around passing the string as -D argument in the shell.
    # we have to additionally add a backslash before a quote to shell-escape it.
    #lib = lib.replace(" ", "\\ ")
    #lib = lib.replace(" ", "\\ ")
    #macro_value += "\"" + lib + "\":\""+  version +"\","
    #macro_value += "*" + lib + "*:*"+  version +"*,"
    # wenn es einen owner gibt bauen wir den mir slash vor den macro value und dann in die cpp defintion PLATFORMIO_USED_LIBRARIES
    if strowner:
        macro_value += "'" + strowner+"/" + lib + "':'"+  version +"',"
    else:
        macro_value += "'" + lib + "':'"+  version +"',"
# chop off last comma
macro_value = macro_value[:-1]
# escape it all in quotes
macro_value = "\\\"" + macro_value + "\\\""

# add to build system as macro
print("PLATFORMIO_USED_LIBRARIES = " + str(macro_value))
projenv.Append(CPPDEFINES=[
  ("PLATFORMIO_USED_LIBRARIES", macro_value)
])

def make_macro_name(lib_name):
    lib_name = lib_name.upper()
    lib_name = lib_name.replace(" ", "_")
#    lib_name = lib_name.replace("-", "_")
    return lib_name

# also add all individual library versions
for lib, version in library_versions.items():
    projenv.Append(CPPDEFINES=[
     ("LIB_VERSION_%s" % make_macro_name(lib) , "\\\"" + version + "\\\"")
    ])
    print("LIB_VERSION_%s = %s" % (make_macro_name(lib), version))

# nun noch die Packages
platform = env.PioPlatform()
used_packages = platform.dump_used_packages()
pkg_metadata = PackageItem(platform.get_dir()).metadata
platform_version = str(pkg_metadata.version if pkg_metadata else platform.version)
platform_version_split = platform_version.split(sep=".")

# projenv.Append(CPPDEFINES=[
#   ("PIO_PLATFORM_VERSION_MAJOR", platform_version[0]),
#   ("PIO_PLATFORM_VERSION_MINOR", platform_version[1]),
#   ("PIO_PLATFORM_VERSION_RELEASE", platform_version[2])
# ])
projenv.Append(CPPDEFINES=[
    ("PIO_PLATFORM_VERSION" ,"\\\"" + platform_version+ "\\\"")
])

print("Platform %s:" % platform_version)


for package in used_packages:
    pio_package_version = package["version"] # e.g. "1.70300.191015"
    pio_package_name = package["name"] # e.g. "toolchain-atmelavr"
    # can fail at decoding and return None if package version is not in semver format
    # in these cases the pio package version is already the decoded version
    # e.g. 1.70300.191015 => 7.3.0
    # e.g. 5.1.0 => None
    pio_decoded_version = get_original_version(pio_package_version)
    print("Name '%s' Version: %s" % (package["name"], str(get_original_version(package["version"]))))
    name_converter = lambda name: name.upper().replace(" ", "_").replace("-", "_")
    # print("Name: %s" % name_converter(pio_package_name))
    projenv.Append(CPPDEFINES=[
     ("PIO_PACKAGE_%s_PKG_VERSION" % name_converter(pio_package_name) , "\\\"" + pio_package_version + "\\\"")
    ])
    if pio_decoded_version is not None: 
        projenv.Append(CPPDEFINES=[
        ("PIO_PACKAGE_%s_DECODED_VERSION" % name_converter(pio_package_name) , "\\\"" + pio_decoded_version + "\\\"")
        ])

# print("Environment: %s" % projenv)
