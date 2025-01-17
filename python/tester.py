#!/usr/bin/env python3
import subprocess
import argparse
import glob
import os
import pickle

# NOTE: MUST BE RUN FROM ROOT FOLDER
# Also, for this to work properly for now, DB names can't have underscores
# Attack video naming scheme: NameInDB_kindofattack.*

def main():
    python_folder = os.path.abspath(os.path.dirname(__file__))
    root_dir = os.path.normpath(os.path.join(python_folder, ".."))
    # Read command-line args
    parser = argparse.ArgumentParser(
        description='Test attack videos with premade database')

    # Positional arguments
    parser.add_argument(
        'srcdir',
        metavar='SOURCEDIR',
        type=str,
        help='path to directory of attack videos')

    parser.add_argument('dbpath', metavar='DBPATH', type=str,
                        help='path to directory to output testing videos')

    parser.add_argument(
        '--frames',
        action='store_true',
        help='match frames instead of scenes; slower but more accurate'
    )
    parser.add_argument(
        '--subimage',
        action='store_true',
        help='additionally looks for a subimage that shares exactly one corner (such as a picture-in-picture attack)'
    )
    parser.add_argument(
        '-shortestmatch',
        metavar = 'SM',
        type=int,
        help='minimum length of matching video clip (in seconds)',
        default=0
    )

    args = parser.parse_args()


    # Create list of videopaths corresponding to videos to query
    vidpaths = []
    if os.path.isfile(args.srcdir):
        vidpaths.append(args.srcdir)
    elif os.path.isdir(args.srcdir):
        vidpaths = glob.glob(args.srcdir + "*")

    # Results in dictionary
    results = dict()

    # Empty dump file at beginning
    with open(os.path.join(root_dir, "results", "dump.txt"), 'w') as f2:
        f2.write("")


    options = [ name for name in os.listdir(args.dbpath) if os.path.isdir(os.path.join(args.dbpath, name)) ]

    for vidpath in vidpaths:
        vidname = os.path.basename(vidpath)
        print(f"Querying: {vidpath}")

        call_args = []
        call_args.append('python3')
        call_args.append(os.path.join(root_dir, 'piracy.py'))
        call_args.append('QUERY')
        call_args.append(args.dbpath)
        call_args.append(vidpath)
        call_args.append('-shortestmatch')
        call_args.append(str(args.shortestmatch))


        if args.frames is True:
            call_args.append("--frames")

        if args.subimage is True:
            call_args.append("--subimage")

        # Run query
        output = subprocess.check_output(call_args)

        # Check if result matches expected
        with open(os.path.join(root_dir, "results", "resultcache.txt")) as file:
            vidlist = file.readlines()
        success = 0
        outstr = "Failure"
        # Name is assumed to contain a single extension ie .mpg
        pirated_from = vidname.split("_")[0]
        for result in vidlist:
            if result.split(".")[0] == pirated_from:
                success = 1
                outstr = "Success"

        # Not meant to be in database
        # if len(vidlist) == 0:
        found = False
        for vid in options:
            if pirated_from in vid:
                found = True
        if not found:
            vidname = f"{pirated_from}_not_in_db.mp4"
            if len(vidlist) == 0:
                success = 1
                outstr = "Success"

        # print(outstr)

        # record 0 for failure, 1 for success
        results[vidname] = success

        with open(os.path.join(root_dir, "results", "dump.txt"), 'a') as f2:
            f2.write(f"{vidname}\n{output}\n{outstr}\n")

    with open(os.path.join(root_dir, "results", "allresults.txt"),'w') as f:
        f.write(str(results))

    with open(os.path.join(root_dir, "results", "allresults.pkl"), 'wb') as f:
        pickle.dump(results, f, pickle.HIGHEST_PROTOCOL)

    print(results)

if __name__ == '__main__':
    main()
