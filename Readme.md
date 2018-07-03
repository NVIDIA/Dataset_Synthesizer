Overview
========

*NDDS* is a UE4 plugin from NVIDIA to empower computer vision researchers to export images with meta-data. NDDS supports images, segmentation, depth, object pose, bounding box, keypoints, and custom stencils at XX FPS. As well as the exporter, we also include different components for generating highly randomized images. This includes light, object, camera position, pose, texture and distractor randomizers, camera path following, etc. Together, these components allow researchers to easily create randomized scenes.
![](./NDDSIntro.png)


Motivation
----------
Training and testing deep learning systems is an expensive and involved task, where traditionally data has to be hand labeled. This is problematic when the task demands expert knowledge or low level control, e.g. robotics grasping manipulation.  In order to overcome these limitations we have been exploring the use of simulators for generating labeled data. We have shown in [1,2] highly randomized synthetic data can be used to train computer vision system for real life applications, thus showing accomplished domain transfer. 

Citation
--------
If you found this project useful, please cite as follow:
> \@article{to2018ndds,
> Author = {Thang To, Jonathan Tremblay, Duncan McKay, Yukie Yamaguchi, Kirby Leung, Adrian Balanon, Jia Cheng, Stan Birchfield},
> url= {https://github.com/NVIDIAGameWorks/NDDS},
> Title = {NDDS: NVIDIA Deep Learning Dataset Synthesizer },
> Year = {2018}
> } 

For further details, please see https://github.com/NVIDIA/Dataset_Synthesizer/Documentation/NDDS.pdf