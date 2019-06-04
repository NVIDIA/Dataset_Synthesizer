NVIDIA Deep learning Dataset Synthesizer (NDDS)
===============================================

Overview
--------

**NDDS** is a UE4 plugin from NVIDIA to empower computer vision researchers to export high-quality synthetic images with metadata. NDDS supports images, segmentation, depth, object pose, bounding box, keypoints, and custom stencils. In addition to the exporter, the plugin includes different components for generating highly randomized images. This randomization includes lighting, objects, camera position, poses, textures, and distractors, as well as camera path following, and so forth. Together, these components allow researchers to easily create randomized scenes for training deep neural networks.

![](./NDDSIntro.png)

_Example of an image generated using NDDS, along with ground truth segmentation, depth, and object poses._ <br> For utilities to help visualize annotation data associated with synthesized images, see the NVIDIA dataset utilities (NVDU) https://github.com/NVIDIA/Dataset_Utilities.

Downloading
-----------
This repository uses gitLFS -- **DO NOT DOWNLOAD AS .ZIP**:

> **First, install git LFS (large file storage):** https://git-lfs.github.com/ , **then lfs clone**.

For further details, please see https://github.com/NVIDIA/Dataset_Synthesizer/blob/master/Documentation/NDDS.pdf

**`RELEASE NOTES: 4.22 known issue`**

<!--This NDDS version uses UE4 version `4.22` with the following release note: -->
> If you are using material randomization with more than 10 objects which change materials every frame, you might encounter a hang when stopping the play-in-editor session.  The capturing process will still work, but the `uniform buffer memory` will keep increasing and when user stops the capture session, it takes UE extended time to release the memory.  If it takes too long after stopping the play-in-editor session, we recommend to simply shutdown the editor and restart it.  The only other workaround is to keep using UE4.21, which requires use of NDDS v1.1.
> 
> This problem is specific to UE4 4.22, as it now automatically uses mesh instancing to improve the performance when rendering a large quantity of meshes.  Now, every time a new mesh is created or its material is changed, the `uniform buffer memory` allocation is increased.          
> 
> This problem affects both DirectX and OpenGL users.  Although Vulkan doesn't get affected by this, Vulkan doesn't capture depth and class segmentation.


Motivation
----------
Training and testing deep learning systems is an expensive and involved task due to the need for hand-labeled data. This is problematic when the task demands expert knowledge or not-so-obvious annotations (e.g., 3D bounding box vertices).  In order to overcome these limitations we have been exploring the use of simulators for generating labeled data. We have shown in [1,2] that highly randomized synthetic data can be used to train computer vision systems for real-world applications, thus showing successful domain transfer.

Citation
--------
If you use this tool in a research project, please cite as follows:
> \@misc{to2018ndds,<br> author = {Thang To and Jonathan Tremblay and Duncan McKay and Yukie Yamaguchi and Kirby Leung and Adrian Balanon and Jia Cheng and William Hodge and Stan Birchfield},<br> note= {\url{ https://github.com/NVIDIA/Dataset_Synthesizer }},<br> title = {{NDDS}: {NVIDIA} Deep Learning Dataset Synthesizer},<br> Year = 2018<br>}


References
----------
[1] J. Tremblay, T. To, A. Molchanov, S. Tyree, J. Kautz, S. Birchfield. Synthetically Trained Neural Networks for Learning Human-Readable Plans from Real-World Demonstrations. In International Conference on Robotics and Automation (ICRA), 2018.

[2] J. Tremblay, T. To, S. Birchfield.  Falling Things:  A Synthetic Dataset for 3D Object Detection and Pose Estimation.  CVPR Workshop on Real World Challenges and New Benchmarks for Deep Learning in Robotic Vision, 2018.