+ 文件来自 [https://github.com/sreiter/stl_reader]
+ test文件夹用于测试

# 基本使用
## 输入stl文件初始化
+ stl_reader::StlMesh <float, unsigned int> mesh("stlfilename.stl");
## 获取网格信息
+ mesh.num_tris() : stl中有多少个三角形块组成
+ mesh.tri_corner_coords(itri, icorner) : stl网格中第itri个三角形的第icorner点
+ mesh.tri_normal(itri) : stl网格第itri个三角形的外法向