FROM python:3.8

RUN apt-get update
RUN apt-get install ffmpeg libsm6 libxext6 libgl1 libglu1 freeglut3 libglew2.1 libepoxy-dev libjpeg62-turbo-dev

WORKDIR /buddy

COPY ./cmake/ /buddy/cmake/
COPY ./include/ /buddy/include/
COPY ./lib/ /buddy/lib/
COPY ./src/ /buddy/src/
COPY ./tests/ /buddy/tests/
COPY ./utils/ /buddy/utils/
COPY ./.clang-format /buddy/.clang-format
COPY ./.cmake-format.py /buddy/.cmake-format.py
COPY ./CMakeLists.txt /buddy/CMakeLists.txt
COPY ./CMakePresets.json /buddy/CMakePresets.json
COPY ./ProjectOptions.cmake /buddy/ProjectOptions.cmake
COPY ./version.txt /buddy/version.txt

RUN python /buddy/utils/bootstrap.py
RUN python /buddy/utils/build.py
RUN pip install -r tests/integration/requirements.txt
CMD pytest tests/integration --firmware build/products/mini_release_boot_4.4.0.bin
