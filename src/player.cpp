#include "player.h"
#include <math.h>
#include <iostream>


void Player::setSpeed(float value_speed) {
	speed = value_speed;
}

float Player::getSpeed() {
	return speed;
}

Player::Player()
{
	position = QVector3D(-20.0f, 4.0f, -3.0f);
	direction = QVector3D(0, 0, 0.7f);
	speed = 0.15f;
}

void Player::init()
{
	m_mesh_player[0].generateMeshFromObjFile("resources/player.obj");
	scale = QVector3D(0.0070f, 0.0070f, 0.0070f);
	m_radius = 0.1f;

	m_name = "Player";
}


void Player::render(GLWidget* glwidget)
{
	m_mesh_player[0].render(glwidget);
}
void Player::update()
{

	// rotation.setY(90-atan2(direction.z(), direction.x()) * 180 / 3.14f);
}