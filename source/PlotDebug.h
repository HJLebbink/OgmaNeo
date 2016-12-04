#pragma once

#include <map>
#include <iostream>		// for cerr and cout
#include <ctime>
#include <string>
#include <fstream>
#include <sstream>      // std::ostringstream
#include <SFML/Graphics.hpp>

#include <neo/Hierarchy.h>
#include <system/SharedLib.h>



using namespace ogmaneo;

namespace plots {

	static std::map<std::string, sf::RenderWindow *> all_windows;

	sf::RenderWindow *getWindowCache(const std::string &name, const Vec2i size, const float scale)
	{
		sf::RenderWindow * window = new sf::RenderWindow();

		if (all_windows.find(name) == all_windows.end()) {
			window->create(sf::VideoMode(static_cast<unsigned int>(size.x * scale), static_cast<unsigned int>(size.y * scale)), name);
			all_windows[name] = window;
		}
		else {
			window = all_windows[name];
		}
		return window;
	}

	void plotImage(
		const sf::Image &image,
		const Vec2i sizeImage,
		const Vec2f pos,
		const float scale,
		sf::RenderWindow &window)
	{
		sf::Texture sdrTex;
		sdrTex.loadFromImage(image);

		sf::Sprite sprite2;
		sprite2.setTexture(sdrTex);
		sprite2.setPosition(pos.x, pos.y + (window.getSize().y - scale * sizeImage.y));
		sprite2.setScale(scale, scale);
		window.draw(sprite2);
	}

	void plotImage(
		const sf::Image &image,
		const Vec2i sizeImage,
		const float scale,
		const std::string &name)
	{
		sf::RenderWindow * window = getWindowCache(name, sizeImage, scale);
		plotImage(image, sizeImage, { 0.0f, 0.0f }, scale, *window);
		window->display();
	}

	void plotImage(
		const ValueField2D &image,
		const float scale,
		sf::RenderWindow &window)
	{
		const Vec2i size = image.getSize();
		sf::Image sdrImg;
		sdrImg.create(size.x, size.y);

		const float maxValue = image.getMaxValue();
		const float minValue = image.getMinValue();
		std::cout << "INFO: plotDebug: maxValue=" << maxValue << "; minvalue=" << minValue << std::endl;

		float offset = 0;
		if ((maxValue > 1.0f) || (minValue < -1.0) || (maxValue == minValue)) {
			std::cout << "WARNING: plotDebug: maxValue=" << maxValue << "; minvalue=" << minValue << std::endl;
			//offset = 1;
		}

		for (int x = 0; x < size.x; ++x) {
			for (int y = 0; y < size.y; ++y) {
				float pixelValue = image.getValue({ x, y }) + offset;
				sf::Color c;
				c.g = 0;
				if (pixelValue > 0) {
					c.r = static_cast<sf::Uint8>(255 * std::min(1.0f, std::max(0.0f, pixelValue)));
				}
				else {
					c.b = static_cast<sf::Uint8>(255 * std::min(1.0f, std::max(0.0f, -pixelValue)));
				}
				sdrImg.setPixel(x, y, c);
			}
		}
		plotImage(sdrImg, size, { 0, 0 }, scale, window);
	}

	void plotImage(
		const ValueField2D &image,
		const float scale,
		const std::string &name)
	{
		sf::RenderWindow * window = getWindowCache(name, image.getSize(), scale);
		plotImage(image, scale, *window);
		window->display();
	}

	void plotImage(
		ComputeSystem &cs,
		const cl::Image2D &img,
		const float scale,
		sf::RenderWindow &window)
	{
		uint32_t width = (uint32_t)img.getImageInfo<CL_IMAGE_WIDTH>();
		uint32_t height = (uint32_t)img.getImageInfo<CL_IMAGE_HEIGHT>();
		uint32_t elementSize = (uint32_t)img.getImageInfo<CL_IMAGE_ELEMENT_SIZE>();

		std::cout << "INFO: PlotDebug:plotImage: width=" << width << "; height=" << height << std::endl;

		std::vector<float> pixels(width * height * (elementSize / sizeof(float)), 0.0f);
		cs.getQueue().enqueueReadImage(img, CL_TRUE, { 0, 0, 0 }, { width, height, 1 }, 0, 0, pixels.data());
		cs.getQueue().finish();

		ValueField2D host(Vec2i{ static_cast<int>(width), static_cast<int>(height) });
		host.getData().swap(pixels);
		plotImage(host, scale, window);
	}

	void plotImage(
		ComputeSystem &cs,
		const cl::Image3D &img,
		const float scale,
		sf::RenderWindow &window)
	{
		const uint32_t width = (uint32_t)img.getImageInfo<CL_IMAGE_WIDTH>();
		const uint32_t height = (uint32_t)img.getImageInfo<CL_IMAGE_HEIGHT>();
		const uint32_t elementSize = (uint32_t)img.getImageInfo<CL_IMAGE_ELEMENT_SIZE>();

		std::vector<float> pixels(width * height * (elementSize / sizeof(float)), 0.0f);
		cs.getQueue().enqueueReadImage(img, CL_TRUE, { 0, 0, 0 }, { width, height, 1 }, 0, 0, pixels.data());
		cs.getQueue().finish();

		ValueField2D host(Vec2i{ static_cast<int>(width), static_cast<int>(height) });
		host.getData().swap(pixels);
		plotImage(host, scale, window);
	}

	void plotImage(
		ComputeSystem &cs,
		const cl::Image2D &img,
		const float scale,
		const std::string &name)
	{
		const int width = (int)img.getImageInfo<CL_IMAGE_WIDTH>();
		const int height = (int)img.getImageInfo<CL_IMAGE_HEIGHT>();
		sf::RenderWindow * window = getWindowCache(name, Vec2i{ width, height }, scale);
		plotImage(cs, img, scale, *window);
		window->display();
	}

	void plotImage(
		ComputeSystem &cs,
		const cl::Image3D &img,
		const float scale,
		const std::string &name)
	{
		const int width = (int)img.getImageInfo<CL_IMAGE_WIDTH>();
		const int height = (int)img.getImageInfo<CL_IMAGE_HEIGHT>();

		sf::RenderWindow * window = getWindowCache(name, Vec2i{ width, height }, scale);
		plotImage(cs, img, scale, *window);
		window->display();
	}
}