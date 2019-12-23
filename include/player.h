#ifndef PLAYER_H
#define PLAYER_H

#include <QVector3D>
#include "gameobject.h"
#include "cmesh.h"
#include "enemy.h"
#include <QElapsedTimer>
//#include <QMatrix4x4>

class Enemy;

class Player : public GameObject
{
public:

	Player();

	QVector3D direction;

	void init();
	void render(GLWidget* glwidget);
	void update();

	void setSpeed(float value_speed);

	float getSpeed();

	CMesh* m_mesh;
	CMesh m_mesh_player[1];

private:

	float speed;

};

#endif // PLAYER_H