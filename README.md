![C/C++ CI](https://github.com/cnagda/id-pirated-vid/workflows/C/C++%20CI/badge.svg?branch=master)

# id-pirated-vid

Our application allows users create a database of known videos and query videos against the
database to detect pirated content. This is done through a command line interface implemented in Python. 
All processing is done in C++.

# Installation

## Installing OpenCV

### Option 1: Using Vagrant

1. First install vagrant. Then from the host machine use vagrant to provision a VM.
2. To start the VM with virtualbox default provider run `vagrant up` within the project directory. This will run provisioning if provisioning has not been started. To restart the vm use `vagrant reload` which will skip provisioning. To connect to the vm, either use `vagrant ssh` or open up virtualbox/VMWare/hyperv and connect graphically. The project folder stays synced to the host machine under `/vagrant` in the ubuntu client OS. We will add an option to run our programs without debug GUI in the future.

### Option 2: Use a Script
Alternatively, run provisionVM.sh in Ubuntu to install dependencies and build opencv with SIFT

### Option3 : Build OpenCV on your own
Alternatively, build opencv itself with opencv_contrib.
My build string:
```
cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=/usr/local \
 -DOPENCV_EXTRA_MODULES_PATH=<opencv_contrib>/modules \
-D WITH_FFMPEG=ON -DOPENCV_ENABLE_NONFREE=ON \
 -D BUILD_EXAMPLES=OFF -D WITH_GTK=ON -D BUILD_opencv_apps=OFF -D BUILD_DOCS=OFF \
 -D BUILD_PERF_TESTS=OFF -D BUILD_TESTS=OFF <opencv_src>
```

## Building project
```
mkdir build
cd build
cmake ..
make
```

## Running Tests
```
mkdir build
cd build
cmake .. -DBUILD_TESTING=ON
make
make test
```

## Installing python dependencies
```
pip install -r requirements.txt
```

# Usage

## `piracy.py`

After building the project, run the command line interface from the root project
folder by executing `piracy.py`

```
usage: piracy.py [-h] {ADD,QUERY,INFO} ...

Check videos for pirated content. Use ADD to create or update a database. Use
QUERY to check an existing database for one or more videos. Use INFO to see
parameters for an existing database. For more information, try ADD, QUERY, or
INFO with the -h option.

optional arguments:
  -h, --help        show this help message and exit

type:
  {ADD,QUERY,INFO}
```

### ADD

Add video(s) to the database or recalculate the database frame/scene vocabulary.
When adding multiple videos with optional arguments, frame/scene vocabulary
will only be recalculated after the last video is added to save time.

```
usage: piracy.py ADD [-h] [-kFrame KF] [-kScene KS] [-thresholdScene TS]
                     dbPath [paths [paths ...]]

positional arguments:
  dbPath              path to database of known videos
  paths               path(s) to directories/files to add

optional arguments:
  -h, --help          show this help message and exit
  -kFrame KF          k value for frame kmeans
  -kScene KS          k value for scene kmeans
  -thresholdScene TS  threshold for inter-scene similarity
```

`kFrame` and `kScene` should be roughly based on the size of your database.

### QUERY

Query the database for each video at `paths` to check for piracy.

```
usage: piracy.py QUERY [-h] [-v] [-shortestmatch SM] [--frames] [--subimage]
                       dbPath paths [paths ...]

positional arguments:
  dbPath             path to database of known videos
  paths              path(s) to directories/files to add

optional arguments:
  -h, --help         show this help message and exit
  -v, --visualize    visualize video matches
  -shortestmatch SM  minimum length of matching video clip (in seconds)
  --frames           match frames instead of scenes; slower but more accurate
  --subimage         additionally looks for a subimage that shares exactly one
                     corner (such as a picture-in-picture attack)
```

#### `--visualize`

If using the `-v` argument, you will be asked for a path to the directory
containing the video files used to construct the database. You will be able
to select and view matching alignments.

#### `--frames`

Using the `--frames` option will be significantly slower. It isn't recommended
for normal use.

#### `-shortestmatch SM`

If you wish to exclude video clip matches that are too short, specify the
`-shortestmatch` with minimum number of seconds.

For example, if you only want matches that are more than 3 seconds, you would
type `-shortestmatch 3` and you will not see matches that are 3 seconds or less.

#### `--subimage`

Some videos may use a subimage of pirated content, such as in a picture-in-picture
attack, so we provide an option to check for a subimage. Note that the subimage
must share a corner with the video.

Using the `--subimage` option will first check for a subimage. If one is found,
we actually run two queries: first with the subimage (called `boxvideo.mp4`) and
second with the rest of the video with a black box on top of the subimage
(this one is called `outervideo.mp4`). These videos are stored in your `results`
folder if you are interested in viewing them. If no subimage is found, the query
proceeds with the entire video.

You can also check for a subimage independently of a query and view the
resulting videos in your `results` folder if a subimage was found, as the
subimage code is in a separate module in `python/subimage_detector.py`.

```
usage: subimage_detector.py [-h] srcpath

Find subimage and save separate files

positional arguments:
  srcpath     path to video with potential subimage

optional arguments:
  -h, --help  show this help message and exit
```



### INFO

Get info about the database

```
usage: piracy.py INFO [-h] dbPath

positional arguments:
  dbPath      path to database of known videos

optional arguments:
  -h, --help  show this help message and exit
```

The info will include the `kFrame`, `kScene`, and `thresholdScene` values for
the database as well as a list of videos in the database.

## `viewer.py`

If you have already run a query and would like to view the results again, find
the corresponding logfile in your `results` folder and run `viewer.py`

```
usage: viewer.py [-h] [-v] [-shortestmatch] logfile querypath

View results of query

positional arguments:
  logfile          path to result logfile
  querypath        path to query video

optional arguments:
  -h, --help       show this help message and exit
  -v, --visualize  visualize video matches
  -shortestmatch   minimum length of matching video clip (in seconds)
```

Similarly to a `piracy.py QUERY`, the `-v` argument lets you view the matching
clips side by side if you know the path to the videos in the database. If you
wish to exclude video clip matches that are short, specify the
`-shortestmatch` with a minimum number of seconds.

## Examples

Create a database from videos in directory `/data/videos/` and compute frame/scene
descriptors and scenes:
```
./piracy.py ADD ./build/database/ ./data/videos/ -kFrame 20000 -kScene 4000 -thresholdScene 30
```

Check to see if video `/data/pirated.mp4` matches any videos in the database:
```
./piracy.py QUERY ./build/database/ ./data/pirated.mp4
```

Later, if you want to view the matches again:
```
./viewer.py ./results/pirated.mp4.csv ./data/pirated.mp4
```

# Visualizations

## Visualize Kmeans
```
./visualize <num_points>
```
This command will save the classified points to visualize.mat and the vocab to vocab.mat

then run gnuplot
```
gnuplot> set xrange[-100:100]
gnuplot> set yrange[-100:100]
gnuplot> plot 'visualize.mat' with points palette pt 7
```
# Evaluation

If you would like to evaluate the success of the piracy detector, you may use
our script `tester.py` in the `python` folder.

## `tester.py`

Note that you should run `tester.py` from the root project directory.

```
$ ./python/tester.py -h

usage: tester.py [-h] [--frames] [--subimage] [-shortestmatch SM]
                 SOURCEDIR DBPATH

Test attack videos with premade database

positional arguments:
  SOURCEDIR          path to directory of attack videos
  DBPATH             path to directory to output testing videos

optional arguments:
  -h, --help         show this help message and exit
  --frames           match frames instead of scenes; slower but more accurate
  --subimage         additionally looks for a subimage that shares exactly one
                     corner (such as a picture-in-picture attack)
  -shortestmatch SM  minimum length of matching video clip (in seconds)
```

## Labeling Attack Videos

The script will tag each video as a "success" or "failure" which is used to
create a report. To take advantage of this functionality, you must label your
attack videos you test.

### If the video is pirated

If the video is pirated, its name should be `"<db name>_<kind of attack>.mp4"`
or an extension of your choice, where `<db name>` is the base name of a video in
the database (without the extension) and `<kind of attack>` is any string that
describes the attack that you want to appear in the report. Multiple videos with
the same `<kind of attack>` will appear in the same row of the report.

The only restriction is that `<db name>` cannot contain underscores. Note that
this means the names of the videos in the database also cannot contain
underscores if you would like to use `tester.py`.

We append `_inserted` at the end of `<kind of attack>` if the pirated clip is
inserted into another video that isn't in the database. We use this to test the
ability of our algorithm to detect random small clips. If you do this, your
report will have an extra column for inserted clips.

### If the video is not pirated

You may use any naming scheme you would like for videos that don't appear in the
database, as the measure of success is whether any match is found or not. To be
properly counted, the video name up until the first underscore (or the whole
video name if there is no underscore) cannot appear anywhere within any of the
names of the videos in the database.

### Examples

Let's say your database has the following folders:
```
video.mp4
example.mp4
sample.mp4
```

The following are acceptable labels for videos that pirate these:
```
video_exact_match.mp4
example_projection.mp4
sample_frame_rate_up.mp4
```

The following are acceptable labels for videos that are not pirated:
```
not_a_pirated_video.mp4
original_video.mp4
```

The following would not be an acceptable label for an original video:
```
vid_notpirated.mp4              "vid" appears in video.mp4
```

## Viewing the Report

After you run `tester.py`, you can see the report by running:
```
$ cd python/ ; python3 make_report.py
```

`make_report.py` can also be run with a single argument specifying a path to
a pickle containing results generated by `tester.py`. `tester.py` automatically
saves this file as `results.pkl` in the `results` folder.

## Generating Attack Videos

If you do not have attack videos in mind to test, you can use our script
`pirater.py` to make attack videos.

```
usage: pirater.py [-h] SOURCEDIR DESTDIR EXTRAVID

Generate testing videos with inserter clips.

positional arguments:
  SOURCEDIR   path to directory of full videos to use as a base
  DESTDIR     path to directory to output testing videos
  EXTRAVID    path to extra video not in the db for insertion

optional arguments:
  -h, --help  show this help message and exit
```

There are currently 17 types of attacks, and a short clip of each attack is also
inserted into a video that isn't in the database, for a total of 34 videos for
every video in `SOURCEDIR`.

Feel free to fork and modify the script for your own purposes. You can easily
comment out attacks or add attacks. You can also comment out the inserted clip
part if you do not want them.

# Credits
Team:

* Kaelan Mikowicz: [@klanmiko](https://github.com/klanmiko)
* Chandni Nagda: [@cnagda](https://github.com/cnagda)
* Emma Kristovich: [@emkristo](https://github.com/emkristo)
* Andrei Blebea: [@The-Boogeyman](https://github.com/The-Boogeyman)

Thank you to our mentors Brian, Lucas, and Troy at Telestream, Inc.

# License
Our project is under the MIT License. See [LICENSE.txt](LICENSE.txt) for the full license.
