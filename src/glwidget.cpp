#include "glwidget.h"
#include "arena.h"
#include "texturemanager.h"
#include <QMouseEvent>
#include <QOpenGLShaderProgram>
#include <QOpenGLFunctions>
#include <QCoreApplication>
#include <math.h>
#include <iostream>
#include <qstack.h>

using namespace std;

GLWidget::GLWidget(QWidget* parent)
	: QOpenGLWidget(parent),
	m_program(nullptr)
{
	setMouseTracking(true);

	QCursor c = cursor();
	c.setShape(Qt::CursorShape::BlankCursor);
	setCursor(c);

	setFocusPolicy(Qt::StrongFocus);

}

GLWidget::~GLWidget()
{
	cleanup();
}

QSize GLWidget::sizeHint() const
{
	return QSize(1000, 800);
}

void GLWidget::cleanup()
{
	if (m_program == nullptr)
		return;
	makeCurrent();

	delete m_program;
	m_program = nullptr;
	doneCurrent();
}
void GLWidget::addObject(GameObject* obj)
{
	obj->init();
	m_gameObjects.push_back(obj);
}

void GLWidget::initCollisionTriangles()
{

	loadHeightMap("resources/Teren.png", QVector3D(0.0f, -1.0f, 0.0f), QVector3D(100.0f, 5.0f, 10.0f),
		QVector2D(10, 10), TextureManager::getTexture("wood"));

	collisionTrianglesMesh.m_primitive = GL_TRIANGLES;
	collisionTrianglesMesh.initVboAndVao();

}

void GLWidget::loadHeightMap(QString filepath, QVector3D center, QVector3D size, QVector2D maxUV, QOpenGLTexture* texture)
{
	QImage img;
	img.load(filepath);

	float boxSizeX = size.x() / img.width();
	float boxSizeZ = size.z() / img.height();

	float offsetX = -boxSizeX * img.width() / 2 + center.x();
	float offsetZ = -boxSizeZ * img.height() / 2;

	float minY = center.y() - size.y() / 2;
	float maxY = center.y() + size.y() / 2;

	int groupSize = (img.width() - 1) * (img.height() - 1) * 2;

	for (int i = 0; i < img.height() - 1; i++) {
		for (int j = 0; j < img.width() - 1; j++)
		{
			float X1 = offsetX + boxSizeX * i;
			float X2 = offsetX + boxSizeX * (i + 1);
			float Z1 = offsetZ + boxSizeZ * j;
			float Z2 = offsetZ + boxSizeZ * (j + 1);

			float Y00 = QColor(img.pixel(j, i)).red() / 255.0f * (maxY - minY) + minY;
			float Y01 = QColor(img.pixel(j + 1, i)).red() / 255.0f * (maxY - minY) + minY;
			float Y10 = QColor(img.pixel(j, i + 1)).red() / 255.0f * (maxY - minY) + minY;
			float Y11 = QColor(img.pixel(j + 1, i + 1)).red() / 255.0f * (maxY - minY) + minY;

			QVector2D texCoord00(maxUV.x() * i / img.width(), maxUV.y() * j / img.height());
			QVector2D texCoord01(maxUV.x() * (i + 1) / img.width(), maxUV.y() * j / img.height());
			QVector2D texCoord10(maxUV.x() * i / img.width(), maxUV.y() * (j + 1) / img.height());
			QVector2D texCoord11(maxUV.x() * (i + 1) / img.width(), maxUV.y() * (j + 1) / img.height());

			addTriangleCollider(QVector3D(X1, Y00, Z1), QVector3D(X1, Y01, Z2), QVector3D(X2, Y11, Z2), groupSize,
				texCoord00, texCoord10, texCoord11, texture);

			addTriangleCollider(QVector3D(X2, Y11, Z2), QVector3D(X2, Y10, Z1), QVector3D(X1, Y00, Z1), groupSize,
				texCoord11, texCoord01, texCoord00, texture);
		}
	}
}

void GLWidget::addTriangleCollider(QVector3D v1, QVector3D v2, QVector3D v3,
	int groupSize, QVector2D uv1, QVector2D uv2,
	QVector2D uv3, QOpenGLTexture* texture)
{
	Triangle t;

	t.v1 = v1;
	t.v2 = v2;
	t.v3 = v3;
	t.texture = texture;
	t.groupSize = groupSize;

	t.n = QVector3D::crossProduct(v1 - v3, v2 - v1).normalized();

	t.A = t.n.x();
	t.B = t.n.y();
	t.C = t.n.z();
	t.D = -(t.A * v1.x() + t.B * v1.y() + t.C * v1.z());

	collisionTriangles.push_back(t);

	collisionTrianglesMesh.add(t.v1, t.n, uv1);
	collisionTrianglesMesh.add(t.v2, t.n, uv2);
	collisionTrianglesMesh.add(t.v3, t.n, uv3);
}

void GLWidget::initializeGL()
{
	initializeOpenGLFunctions();
	glClearColor(0.1f, 0.2f, 0.3f, 1);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);

	game_is_defeted = false;

	CMesh::loadAllMeshes();
	TextureManager::init();
	m_program = new QOpenGLShaderProgram;
	m_program->addShaderFromSourceFile(QOpenGLShader::Vertex, "resources/shader.vs");
	m_program->addShaderFromSourceFile(QOpenGLShader::Fragment, "resources/shader.fs");
	m_program->bindAttributeLocation("vertex", 0);
	m_program->bindAttributeLocation("normal", 1);
	m_program->link();

	m_program->bind();
	m_projMatrixLoc = m_program->uniformLocation("projMatrix");
	m_viewMatrixLoc = m_program->uniformLocation("viewMatrix");
	m_modelMatrixLoc = m_program->uniformLocation("modelMatrix");
	m_hasTextureLoc = m_program->uniformLocation("hasTexture");
	m_cameraPositionLoc = m_program->uniformLocation("cameraPosition");

	m_materialLoc.ambient = m_program->uniformLocation("material.ambient");
	m_materialLoc.diffuse = m_program->uniformLocation("material.diffuse");
	m_materialLoc.specular = m_program->uniformLocation("material.specular");
	m_materialLoc.shininess = m_program->uniformLocation("material.shininess");

	for (int i = 0; i < MAX_LIGHTS; i++) {

		m_lightLoc[i].position = m_program->uniformLocation(QString().asprintf("light[%d].position", i));
		m_lightLoc[i].ambient = m_program->uniformLocation(QString().asprintf("light[%d].ambient", i));
		m_lightLoc[i].diffuse = m_program->uniformLocation(QString().asprintf("light[%d].diffuse", i));
		m_lightLoc[i].specular = m_program->uniformLocation(QString().asprintf("light[%d].specular", i));
		m_lightLoc[i].isActive = m_program->uniformLocation(QString().asprintf("light[%d].isActive", i));
		m_lightLoc[i].attenuation = m_program->uniformLocation(QString().asprintf("light[%d].attenuation", i));

	}

	m_lastPos = QPoint(0, 0);

	for (int i = 0; i < 256; i++) {
		m_keyState[i] = false;
	}

	m_program->release();


	m_program_hud = new QOpenGLShaderProgram;
	m_program_hud->addShaderFromSourceFile(QOpenGLShader::Vertex, "resources/shader_hud.vs");
	m_program_hud->addShaderFromSourceFile(QOpenGLShader::Fragment, "resources/shader_hud.fs");
	m_program_hud->bindAttributeLocation("vertex", 0);
	m_program_hud->bindAttributeLocation("normal", 1);
	m_program_hud->bindAttributeLocation("uvCoord", 2);
	m_program_hud->link();

	m_program_hud->bind();

	m_resolutionLoc_hud = m_program_hud->uniformLocation("resolution");
	m_color_hud = m_program_hud->uniformLocation("color");
	m_hasTextureLoc_hud = m_program_hud->uniformLocation("hasTexture");
	m_rectangleLoc_hud.xPos = m_program_hud->uniformLocation("rect.xPos");
	m_rectangleLoc_hud.yPos = m_program_hud->uniformLocation("rect.yPos");
	m_rectangleLoc_hud.width = m_program_hud->uniformLocation("rect.width");
	m_rectangleLoc_hud.height = m_program_hud->uniformLocation("rect.height");

	m_program_hud->release();

	lastUpdateTime = 0;
	timer.start();
	FPS = 60;

	initCollisionTriangles();

	addObject(&m_player);

}
void GLWidget::updateGL()
{


	QCursor::setPos(mapToGlobal(QPoint(width() / 2, height() / 2)));

	m_player.position.setX(m_player.position.x() + m_player.direction.z() * m_player.getSpeed());



	//Moving up

	if (m_keyState[Qt::Key_Space]) {

		m_player.position.setY(m_player.position.y() + 0.20f);
	}

	//Moving up

	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		GameObject* obj = m_gameObjects[i];

		obj->previousPosition = obj->position;      // Previous item position

// We compare every object with everyone
		for (int j = 0; j < m_gameObjects.size(); j++)
		{

			if (i == j) // We do not compare objects with ourselves
				continue;

			GameObject* obj2 = m_gameObjects[j];
			// We count the vector from the position of one object to another
			QVector3D v = obj->position - obj2->position;

			// The length of this vector is the distance between the centers of the objects

			float d = v.length();
			// We compare with the sum of the rays

			if (d < (obj->m_radius + obj2->m_radius))
			{
				std::string name1 = obj->m_name;
				std::string name2 = obj2->m_name;

				GameObject* o1 = obj;
				GameObject* o2 = obj2;

				// o2->position += ( v - o1->position ) * 0.3f;

				if (strcmp(name1.c_str(), name2.c_str()) > 0)
				{
					o1 = obj2;
					o2 = obj;
					v = -v;
				}

				{

					o1->position = o1->position + v * (d / 2); //collision improvement
					o2->position = o2->position - v * (d / 2);

					v.normalize();
				}
			}
		}

		obj->position.setY(obj->position.y() - 0.10f); //Gravitation

		obj->update();

	}

	for (unsigned int i = 0; i < m_gameObjects.size(); i++) {
		GameObject* obj = m_gameObjects[i];

		for (int j = 0; j < collisionTriangles.size(); j++) {

			Triangle tr = collisionTriangles[j];

			float currDist = tr.A * obj->position.x() + tr.B * obj->position.y() + tr.C * obj->position.z() + tr.D;
			float prevDist = tr.A * obj->previousPosition.x() + tr.B * obj->previousPosition.y() + tr.C * obj->previousPosition.z() + tr.D;

			if ((currDist * prevDist < 0) || abs(currDist) < obj->m_radius)
			{
				//Projection of the object's position on the plane
				QVector3D p = obj->position - tr.n * currDist;

				//Shift the point to the center of the triangle with a collider radius
				QVector3D r = (tr.v1 + tr.v2 + tr.v3) * (1.0f / 3.0f) - p;
				r = r.normalized();
				p = p + r * obj->m_radius;

				//Calculation of v, m, u - barycentric coordinates
				QVector3D v0 = tr.v2 - tr.v1, v1 = tr.v3 - tr.v1, v2 = p - tr.v1;

				float d00 = QVector3D::dotProduct(v0, v0);
				float d01 = QVector3D::dotProduct(v0, v1);
				float d11 = QVector3D::dotProduct(v1, v1);
				float d20 = QVector3D::dotProduct(v2, v0);
				float d21 = QVector3D::dotProduct(v2, v1);
				float denom = d00 * d11 - d01 * d01;

				float v = (d11 * d20 - d01 * d21) / denom;
				float w = (d00 * d21 - d01 * d20) / denom;
				float u = 1.0f - v - w;

				// Checking if the point is in the middle of the triangle
				if (v >= 0 && w >= 0 && (v + w) <= 1) {

					float d = obj->m_radius - currDist;
					obj->position = obj->position + tr.n * d;
					game_is_defeted = true;


				}
			}
		}
	}
}

void GLWidget::paintGL()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	QStack<QMatrix4x4> worldMatrixStack;

	m_lights[0].position = QVector3D(-3.0f, 1.0f, -4.0f);
	m_lights[0].ambient = QVector3D(0.1f, 0.1f, 0.1f);
	m_lights[0].diffuse = QVector3D(0.3f, 0.3f, 0.3f);
	m_lights[0].specular = QVector3D(0.1f, 0.1f, 0.1f);
	m_lights[0].isActive = true;
	m_lights[0].attenuation = 0.5f;


	m_lights[1].position = QVector3D(3.0f, 1.0f, -4.0f);
	m_lights[1].ambient = QVector3D(0.1f, 0.1f, 0.1f);
	m_lights[1].diffuse = QVector3D(0.3f, 0.3f, 0.3f);
	m_lights[1].specular = QVector3D(0.1f, 0.1f, 0.1f);
	m_lights[1].isActive = true;
	m_lights[1].attenuation = 0.5f;

	m_program->bind();

	m_program->setUniformValue(m_cameraPositionLoc, m_player.position - m_camDistance * m_player.direction);

	setLights();

	m_camera.setToIdentity();
	m_world.setToIdentity();

	m_camera.lookAt(m_player.position - (m_camDistance + 6) * m_player.direction, m_player.position, QVector3D(0, 1, 0));

	for (int i = 0; i < m_gameObjects.size(); i++) {

		GameObject* obj = m_gameObjects[i];

		m_program->setUniformValue(m_materialLoc.ambient, obj->material.ambient);
		m_program->setUniformValue(m_materialLoc.diffuse, obj->material.diffuse);
		m_program->setUniformValue(m_materialLoc.specular, obj->material.specular);
		m_program->setUniformValue(m_materialLoc.shininess, obj->material.shininess);

		if (obj->m_texture != nullptr)
		{
			m_program->setAttributeValue(m_hasTextureLoc, 1);
			obj->m_texture->bind();
		}
		else
		{
			m_program->setAttributeValue(m_hasTextureLoc, 0);
		}

		worldMatrixStack.push(m_world);
		m_world.translate(obj->position);

		m_world.rotate(obj->rotation.x(), 1, 0, 0);
		m_world.rotate(obj->rotation.y(), 0, 1, 0);
		m_world.rotate(obj->rotation.z(), 0, 0, 1);

		m_world.scale(obj->scale);
		setTransforms();
		obj->render(this);
		m_world = worldMatrixStack.pop();

	}

	for (int i = 0; i < m_gameObjects.size(); i++)
	{
		GameObject* obj = m_gameObjects[i];
		obj->update();
	}


	for (int i = 0; i < collisionTriangles.size(); ) {

		Triangle triangle = collisionTriangles[i];

		m_program->setUniformValue(m_materialLoc.ambient, QVector3D(1.0f, 1.0f, 1.0f));
		m_program->setUniformValue(m_materialLoc.diffuse, QVector3D(1.0f, 1.0f, 1.0f));
		m_program->setUniformValue(m_materialLoc.specular, QVector3D(1.0f, 1.0f, 1.0f));
		m_program->setUniformValue(m_materialLoc.shininess, 1.0f);

		if (triangle.texture != nullptr) {
			m_program->setUniformValue(m_hasTextureLoc, 1);
			triangle.texture->bind();
		}
		else
		{
			m_program->setUniformValue(m_hasTextureLoc, 0);
		}

		worldMatrixStack.push(m_world);
		m_world.translate(QVector3D(0, 0, 0));
		m_world.rotate(0, 1, 0, 0);
		m_world.rotate(0, 0, 1, 0);
		m_world.rotate(0, 0, 0, 1);
		m_world.scale(QVector3D(1, 1, 1));
		setTransforms();
		collisionTrianglesMesh.render(this, i * 3, triangle.groupSize * 3);
		m_world = worldMatrixStack.pop();

		i += triangle.groupSize;
	}

	CMesh* skydomeMesh = CMesh::m_meshes["skydome"];
	m_lights[0].ambient = QVector3D(1.0f, 1.0f, 1.0f);

	setLights();

	m_program->setUniformValue(m_materialLoc.ambient, QVector3D(1.0f, 1.0f, 1.0f));
	m_program->setUniformValue(m_materialLoc.diffuse, QVector3D(0.0f, 0.0f, 0.0f));
	m_program->setUniformValue(m_materialLoc.specular, QVector3D(0.0f, 0.0f, 0.0f));
	m_program->setUniformValue(m_materialLoc.shininess, 0.0f);

	m_program->setUniformValue(m_hasTextureLoc, 1);
	TextureManager::getTexture("skydome")->bind();

	worldMatrixStack.push(m_world);
	m_world.translate(m_player.position);
	m_world.rotate(0, 1, 0, 0);
	m_world.rotate(timer.elapsed() * 0.005f, 0, 1, 0);
	m_world.rotate(0, 0, 0, 1);
	m_world.scale(20.0f);
	setTransforms();
	skydomeMesh->render(this);
	m_world = worldMatrixStack.pop();
	m_program->release();

	float timerTime = timer.elapsed() * 0.001f;
	float deltaTime = timerTime - lastUpdateTime;

	if (deltaTime >= (1.0f / FPS)) {
		updateGL();
		lastUpdateTime = timerTime;
	}

	paintHUD();

	update();
}

void GLWidget::setLights()
{
	for (int i = 0; i < MAX_LIGHTS; i++) {
		m_program->setUniformValue(m_lightLoc[i].position, m_lights[i].position);
		m_program->setUniformValue(m_lightLoc[i].ambient, m_lights[i].ambient);
		m_program->setUniformValue(m_lightLoc[i].diffuse, m_lights[i].diffuse);
		m_program->setUniformValue(m_lightLoc[i].specular, m_lights[i].specular);
		m_program->setUniformValue(m_lightLoc[i].isActive, m_lights[i].isActive);
		m_program->setUniformValue(m_lightLoc[i].attenuation, m_lights[i].attenuation);
	}
}

void GLWidget::setRectangle(float xPos, float yPos, float width, float height, QVector3D color, QOpenGLTexture* texture)
{
	m_program_hud->setUniformValue(m_rectangleLoc_hud.xPos, xPos);
	m_program_hud->setUniformValue(m_rectangleLoc_hud.yPos, yPos);
	m_program_hud->setUniformValue(m_rectangleLoc_hud.width, width);
	m_program_hud->setUniformValue(m_rectangleLoc_hud.height, height);
	m_program_hud->setUniformValue(m_color_hud, color);

	if (texture != nullptr)
	{
		m_program_hud->setUniformValue(m_hasTextureLoc_hud, 1);
		texture->bind();
	}
	else
	{
		m_program_hud->setUniformValue(m_hasTextureLoc_hud, 0);
	}
}

void GLWidget::paintHUD()
{
	CMesh* rectMesh = CMesh::m_meshes["rect"];

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC0_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_program_hud->bind();
	m_program_hud->setUniformValue(m_resolutionLoc_hud, m_resolution);

	if (game_is_defeted == true) {
		setRectangle(300, 100, 1100, 800, QVector3D(1, 1, 1), TextureManager::getTexture("game_over"));
		rectMesh->render(this);
	}

	glDisable(GL_BLEND);
	m_program_hud->release();
}

void GLWidget::setTransforms(void)
{
	m_program->setUniformValue(m_projMatrixLoc, m_proj);
	m_program->setUniformValue(m_viewMatrixLoc, m_camera);
	m_program->setUniformValue(m_modelMatrixLoc, m_world);
}

void GLWidget::resizeGL(int w, int h)
{
	m_proj.setToIdentity();
	m_proj.perspective(60.0f, GLfloat(w) / h, 0.01f, 100.0f);

	m_resolution = QVector2D(w, h);
}

void GLWidget::mousePressEvent(QMouseEvent* event)
{
	m_lastPos = event->pos();
	if (event->type() == QMouseEvent::MouseButtonPress) {

	}
}

void GLWidget::mouseMoveEvent(QMouseEvent* event)
{
	int dx = event->x() - width() / 2;
	int dy = event->y() - height() / 2;

	float phi = atan2(m_player.direction.z(), m_player.direction.x());
	float theta = acos(m_player.direction.y());

	phi = phi + dx * 0.0009;
	theta = theta + dy * 0.0009;

	if (theta < 0.01) theta = 0.01;
	if (theta > 3.14) theta = 3.14;


}

void GLWidget::keyPressEvent(QKeyEvent* e)
{
	if (e->key() == Qt::Key_Escape)
		exit(0);
	else
		QWidget::keyPressEvent(e);

	if (e->key() >= 0 && e->key() <= 255)
		m_keyState[e->key()] = true;
}

void GLWidget::keyReleaseEvent(QKeyEvent* e)
{
	if (e->key() >= 0 && e->key() <= 255)
		m_keyState[e->key()] = false;
}
