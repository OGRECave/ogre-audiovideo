FROM quay.io/pypa/manylinux_2_28_x86_64:latest

ARG OGRE_TAG
ARG JOBS=4

COPY ci/build-ogre.cmake /tmp/build-ogre.cmake
RUN OGRE_TAG="${OGRE_TAG}" \
    OGRE_PREFIX=/opt/ogre \
    OGRE_BUILD_DIR=/tmp/ogre-build \
    JOBS="${JOBS}" \
    cmake -P /tmp/build-ogre.cmake \
 && rm -rf /tmp/ogre-build /tmp/build-ogre.cmake

# OGRE installs its config to <prefix>/lib/OGRE/cmake and SDL2 to
# <prefix>/deps/lib/cmake/SDL2 (<prefix>/deps/cmake on Windows); both are
# found through CMAKE_PREFIX_PATH.
ENV CMAKE_PREFIX_PATH="/opt/ogre;/opt/ogre/deps"
ENV CMAKE_BUILD_PARALLEL_LEVEL=4

WORKDIR /workspace
CMD PATH="/opt/python/$PYVER/bin/:$PATH" pip wheel . --no-deps --verbose
