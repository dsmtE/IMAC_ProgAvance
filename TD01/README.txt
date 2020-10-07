Boids Tower Defense (BTD)
-------------------------

Test project for students. Goals:
	- Advanced programming
	- Integrate a project with already existing code
	- Pathfinding using boost.graph
	- Animaition (clouds, day/night light cycle...)
	- And a few other things
Not a real game actually, until code is added to it.



Keyboard
o,p,pgUp,pgDn: rotate camera
s: stop constructor

Mouse
click: move constructor to point (if possible)
B: build a turret (if possible)
Wheel: zoom


Maintenance
-----------

Usually, modify master and merge branches that way:
master -> td4 -> td3 -> td2 -> td1
Optionally: td1 to 4 -> master, 'ours' strategy (= record merge only)

When performing a change on another branch (say td2):
td2 (previous commit) -> master, 'ours' strategy (= record merge only)
td2 -> master
master -> td4 -> td3 -> td2 -> td1
