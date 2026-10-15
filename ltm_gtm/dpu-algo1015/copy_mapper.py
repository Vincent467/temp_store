import os
import shutil

def copy_files(src_root, mapper_root):
    for src_dir in os.listdir(src_root):
        src_path = os.path.join(src_root, src_dir)
        if os.path.isdir(src_path):
            for sub_src_dir in os.listdir(src_path):
                sub_src_path = os.path.join(src_path, sub_src_dir)
                if os.path.isdir(sub_src_path):
                    mapper_path = os.path.join(mapper_root, src_dir + "/" + sub_src_dir)
                    if os.path.exists(mapper_path) and os.path.isdir(mapper_path):
                        for file_name in os.listdir(mapper_path):
                            mapper_file = os.path.join(mapper_path, file_name)
                            src_file = os.path.join(sub_src_path, file_name)
                            if os.path.isfile(mapper_file):
                                shutil.copy2(mapper_file, src_file)
                                print("copy %s to %s"%(mapper_file, src_file))

confusion_src_folder = './src/confusion_algo'
src_folder = './src/algo'
mapper_folder = './mapper'

copy_files(confusion_src_folder, mapper_folder)
copy_files(src_folder, mapper_folder)
