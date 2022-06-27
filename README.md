<div id="top"></div>

<!-- ABOUT THE PROJECT -->
## About The Project
The `qdsDataInterface` Project implements the core functionality of a TRUMPF Quality Data Storage (QDS) buffer. It offers raw interfaces (C++ headers) to communicate with the buffer for adding and retrieving QDS data.

Implemented core functionalities are:
- Parse JSON string representation of QDS data
- Validate JSON
- Serialize QDS data as JSON string
- Store and manage QDS data in a buffer
- Handle References (REF data type)
- Load QDS data values (VALUE data type) from the file system
- Differing logic based on Counter Mode

The goal is to have a core library which can be used in various projects, each with its own communication stack (e.g. gRPC, OPC-UA, REST) but same requirements for handling QDS data.

### Built With
- [CMake](https://cmake.org/)
- [GNU Compiler Collection](https://gcc.gnu.org/) (Linux)
- [Microsoft Visual C++ (MSVC) compiler toolset](https://docs.microsoft.com/en-us/cpp/?view=msvc-170) (Windows)
- [Boost C++ Libraries](https://www.boost.org/)
- [GoogleTest](https://github.com/google/googletest) (for Unit Testing)
- [GIT](https://git-scm.com/) (for Unit Testing)

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- GETTING STARTED -->
## Getting Started
This section walks you through the steps of building a functioning library for use in your project (both Linux and Windows)

### Prerequisites

##### CMake
At least CMake 3.14 is required.

##### GNU Compiler Collection (Linux)
At least GCC/G++ 7.1 is required (C++17 support).

##### Microsoft Visual C++ (MSVC) compiler toolset (Windows)
At least VS 2017 15.0 is required (C++17 support).

##### Boost
At least Boost 1.75 is required.

Download, build and install on Linux:
```
$ wget -O boost_1_75_0.tar.gz https://sourceforge.net/projects/boost/files/boost/1.75.0/boost_1_75_0.tar.gz/download
$ tar xzvf boost_1_75_0.tar.gz
$ cd boost_1_75_0
$ ./bootstrap.sh
$ ./b2 cxxflags=-fPIC -a --with-json --prefix=<install_dir> install
```

Download, build and install on Windows:
```
$ Invoke-WebRequest -UserAgent "Wget" -Uri https://sourceforge.net/projects/boost/files/boost/1.75.0/boost_1_75_0.tar.gz/download -OutFile boost_1_75_0.tar.gz
$ tar xzvf boost_1_75_0.tar.gz
$ cd boost_1_75_0
$ .\bootstrap.bat
$ .\b2 -a --with-json --prefix=<boost_install_dir> install
```

##### GoogleTest (for Unit Testing)
CMake will automatically download and install GoogleTest in the local build folder. For this to work, `git` must be installed on the system.

If you would like to disable testing you can pass the flag `-DTESTING=OFF` to the CMake command.

<p align="right">(<a href="#top">back to top</a>)</p>

### Build
Run the `cmake` command to generate a Makefile for the project:
```
$ mkdir build && cd build/
$ cmake ..
```
##### Config Options
Here are some useful options you can add to the `cmake` command above:
- `BOOST_ROOT`: The path to the boost root directory
- `CMAKE_INSTALL_PREFIX`: Install directory
- `BUILD_SHARED_LIBS`: Build project as static or shared library (values: ON/OFF, default is OFF)
- `TESTING`: Enable or disable unit tests (values: ON/OFF, default is ON)

For example if you installed boost to a non-default location and want to build a shared library, install it under "/usr" and disable testing, you would call `cmake` like this:
```
$ cmake .. -DBOOST_ROOT=<boost_install_dir> -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_SHARED_LIBS=ON -DTESTING=OFF
```

After configuration has succeeded, you can build the project:
```
$ cmake --build . --config Release
```

<p align="right">(<a href="#top">back to top</a>)</p>

### Testing
If testing was not disabled during build, all tests can be run like this:
```
$ ctest
Test project qdsDataInterface/build
      Start  1: RingBufferTest.SimplePushRead
 1/33 Test  #1: RingBufferTest.SimplePushRead ..................   Passed    0.00 sec
      Start  2: RingBufferTest.ValidatePush
 2/33 Test  #2: RingBufferTest.ValidatePush ....................   Passed    0.00 sec
...
      Start 33: DataValidatorTest.OnValueValue
33/33 Test #33: DataValidatorTest.OnValueValue .................   Passed    0.00 sec

100% tests passed, 0 tests failed out of 33

Total Test time (real) =   0.23 sec
```

### Install
Linux:
```
$ make install
```

Windows:
```
$ cmake --install .
```

This will install the library (static or shared) and all public headers to the directory specified during build. Depending on the environment, the install command might need to run with `sudo` on Linux.

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- USAGE EXAMPLES -->
## Usage
### Create Data Source object
Interacting with the QDS buffer happens through the `DataSource` object. First, create the object with the help of the `DataSourceFactory` class:
##### main.cpp
```
#include <data_source_factory.hpp>

std::shared_ptr<qds_buffer::core::IDataSourceInOut> data_source = 
                                    qds_buffer::core::DataSourceFactory::CreateDataSource();
```
`data_source` is a shared pointer to the `IDataSourceInOut` interface of the object. This is one of three interfaces for object access:
- `IDataSourceIn`: Input interface useful for producers of QDS data
- `IDataSourceOut`: Output interface useful for consumers of QDS data
- `IDataSourceInOut`: Combines input and output interfaces

Next, pass the `data_source` to the producer and consumer. Limit the access to `data_source` by casting to `IDataSourceIn` or `IDataSourceOut`:
##### producer.cpp
```
#include <i_data_source_in.hpp>

class Producer {
  Producer(std::shared_ptr<core::IDataSourceIn> data_source);
}
```
##### consumer.cpp
```
#include <i_data_source_out.hpp>

class Consumer {
  Consumer(std::shared_ptr<core::IDataSourceOut> data_source);
}
```
##### main.cpp
```
Producer producer{data_source};
Consumer consumer{data_source};
```

<p align="right">(<a href="#top">back to top</a>)</p>

### Add QDS data
The producer can add new QDS data to the data source:
##### producer.cpp
```
data_source->Add(123, "[{\"NAME\":\"Program\",\"TYPE\":\"STRING\",\"VALUE\":\"test\"}]");
```

### Get QDS data
The consumer can retrieve existing QDS data by iterating over the data source:
##### consumer.cpp
```
std::shared_lock lock(data_source_->GetBufferSharedMutex());
for (BufferEntry& entry : *data_source_) {
  int64_t id = entry.id_;
  auto measurements = entry.measurements_;
  entry.locked_ = true;
}
```
IMPORTANT: Always lock the shared mutex of the buffer before accessing the iterator. Don't forget to unlock after you are done.

### Delete QDS data
After retrieving the data, the consumer can delete the data:
##### consumer.cpp
```
data_source_->Delete(123);
```

### Next steps
All available input/output methods can be found in the interfaces `i_data_source_in.hpp` and `i_data_source_out.hpp`.

<p align="right">(<a href="#top">back to top</a>)</p>

## Note on Thread-Safety
The Data Source object has been designed to be thread-safe. Concurrent access from multiple threads is guaranteed to work. However, when iterating through the data source (see [Get QDS data](#get-qds-data)), it is necessary to lock the shared mutex of the buffer, otherwise thread-safety is no longer guaranteed.

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this project better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

<p align="right">(<a href="#top">back to top</a>)</p>

<!-- LICENSE -->
## License

Distributed under the MPL-2.0 License. See `LICENSE` for more information.

<p align="right">(<a href="#top">back to top</a>)</p>
