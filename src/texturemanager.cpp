#include "texturemanager.h"

std::map<std::string, QOpenGLTexture*> TextureManager::m_textures;

TextureManager::TextureManager()
{

}

void TextureManager::init()
{
	m_textures["brick"] = new QOpenGLTexture(QImage("resources/brick.jpg"));
	m_textures["wood"] = new QOpenGLTexture(QImage("resources/wood.jpeg"));
	m_textures["grass"] = new QOpenGLTexture(QImage("resources/grass.jpg"));
	m_textures["skydome"] = new QOpenGLTexture(QImage("resources/skydome.png"));

	m_textures["game_over"] = new QOpenGLTexture(QImage("resources/game_over.jpg"));

}

QOpenGLTexture* TextureManager::getTexture(std::string name)
{
	return m_textures[name];
}