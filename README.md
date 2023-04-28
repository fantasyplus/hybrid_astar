# Hybrid A*

## ompl需要单独编译
```
cd ompl
./build_ompl.sh
```

## 编译流程
```
rosdep install --from-paths src --ignore-src --rosdistro=${ROS_DISTRO} -y
```

```
catkin_make
```
```
source devel/setup.bash
```

```
roslaunch hybrid_astar_node hybrid_astar_static.launch
```

