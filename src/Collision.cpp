/*
* File:   collision.cpp
* Author: Nick (original version), ahnonay (SFML2 compatibility)
*/

#include <SFML/Graphics.hpp>
#include <map>
#include "Collision.h"

namespace Collision
{
	class BitmaskManager
	{
	public:
		~BitmaskManager() {
			std::map<const sf::Texture*, std::uint8_t*>::const_iterator end = Bitmasks.end();
			for (std::map<const sf::Texture*, std::uint8_t*>::const_iterator iter = Bitmasks.begin(); iter != end; iter++)
				delete[] iter->second;
		}

		std::uint8_t GetPixel(const std::uint8_t* mask, const sf::Texture* tex, unsigned int x, unsigned int y) {
			if (x>tex->getSize().x || y>tex->getSize().y)
				return 0;

			return mask[x + y*tex->getSize().x];
		}

		std::uint8_t* GetMask(const sf::Texture* tex) {
			std::uint8_t* mask;
			std::map<const sf::Texture*, std::uint8_t*>::iterator pair = Bitmasks.find(tex);
			if (pair == Bitmasks.end())
			{
				sf::Image img = tex->copyToImage();
				mask = CreateMask(tex, img);
			}
			else
				mask = pair->second;

			return mask;
		}

		std::uint8_t* CreateMask(const sf::Texture* tex, const sf::Image& img) {
			std::uint8_t* mask = new std::uint8_t[tex->getSize().y*tex->getSize().x];

			for (unsigned int y = 0; y<tex->getSize().y; y++)
			{
				for (unsigned int x = 0; x<tex->getSize().x; x++)
					mask[x + y*tex->getSize().x] = img.getPixel(x, y).a;
			}

			Bitmasks.insert(std::pair<const sf::Texture*, std::uint8_t*>(tex, mask));

			return mask;
		}
	private:
		std::map<const sf::Texture*, std::uint8_t*> Bitmasks;
	};

	BitmaskManager Bitmasks;

	bool PixelPerfectTest(const sf::Sprite& Object1, const sf::Sprite& Object2, std::uint8_t AlphaLimit) {
		sf::FloatRect Intersection;
		if (Object1.getGlobalBounds().intersects(Object2.getGlobalBounds(), Intersection)) {
			sf::IntRect O1SubRect = Object1.getTextureRect();
			sf::IntRect O2SubRect = Object2.getTextureRect();

			std::uint8_t* mask1 = Bitmasks.GetMask(Object1.getTexture());
			std::uint8_t* mask2 = Bitmasks.GetMask(Object2.getTexture());

			// Loop through our pixels
			for (float i = Intersection.position.x; i < Intersection.position.x + Intersection.size.x; i++) {
				for (float j = Intersection.position.y; j < Intersection.position.y + Intersection.size.y; j++) {

					sf::Vector2f o1v = Object1.getInverseTransform().transformPoint(sf::Vector2f(i, j));
					sf::Vector2f o2v = Object2.getInverseTransform().transformPoint(sf::Vector2f(i, j));

					// Make sure pixels fall within the sprite's subrect
					if (o1v.x > 0 && o1v.y > 0 && o2v.x > 0 && o2v.y > 0 &&
						o1v.x < O1SubRect.size.x && o1v.y < O1SubRect.size.y &&
						o2v.x < O2SubRect.size.x && o2v.y < O2SubRect.size.y) {

						if (Bitmasks.GetPixel(mask1, Object1.getTexture(), (int)(o1v.x) + O1SubRect.position.x, (int)(o1v.y) + O1SubRect.position.y) > AlphaLimit &&
							Bitmasks.GetPixel(mask2, Object2.getTexture(), (int)(o2v.x) + O2SubRect.position.x, (int)(o2v.y) + O2SubRect.position.y) > AlphaLimit)
							return true;

					}
				}
			}
		}
		return false;
	}

	bool CreateTextureAndBitmask(sf::Texture &LoadInto, const std::string& Filename)
	{
		sf::Image img;
		if (!img.loadFromFile(Filename))
			return false;
		if (!LoadInto.loadFromImage(img))
			return false;

		Bitmasks.CreateMask(&LoadInto, img);
		return true;
	}

	sf::Vector2f GetSpriteCenter(const sf::Sprite& Object)
	{
		sf::FloatRect AABB = Object.getGlobalBounds();
		return sf::Vector2f(AABB.size.x + AABB.size.y / 2.f, AABB.position.y + AABB.size.y / 2.f);
	}
	sf::Vector2f GetSpriteSize(const sf::Sprite& Object)
	{
		sf::IntRect OriginalSize = Object.getTextureRect();
		sf::Vector2f Scale = Object.getScale();
		return sf::Vector2f(OriginalSize.size.x*Scale.x, OriginalSize.size.y*Scale.y);
	}

	bool CircleTest(const sf::Sprite& Object1, const sf::Sprite& Object2) {
		sf::Vector2f Obj1Size = GetSpriteSize(Object1);
		sf::Vector2f Obj2Size = GetSpriteSize(Object2);
		float Radius1 = (Obj1Size.x + Obj1Size.y) / 4;
		float Radius2 = (Obj2Size.x + Obj2Size.y) / 4;

		sf::Vector2f Distance = GetSpriteCenter(Object1) - GetSpriteCenter(Object2);

		return (Distance.x * Distance.x + Distance.y * Distance.y <= (Radius1 + Radius2) * (Radius1 + Radius2));
	}

	class OrientedBoundingBox // Used in the BoundingBoxTest
	{
	public:
		OrientedBoundingBox(const sf::Sprite& Object) // Calculate the four points of the OBB from a transformed (scaled, rotated...) sprite
		{
			sf::Transform trans = Object.getTransform();
			sf::IntRect local = Object.getTextureRect();
			Points[0] = trans.transformPoint(sf::Vector2f(0.f, 0.f));
			Points[1] = trans.transformPoint(sf::Vector2f((float)local.size.x, 0.f));
			Points[2] = trans.transformPoint(sf::Vector2f((float)local.size.x, (float)local.size.y));
			Points[3] = trans.transformPoint(sf::Vector2f(0.f, (float)local.size.y));
		}

		sf::Vector2f Points[4];

		void ProjectOntoAxis(const sf::Vector2f& Axis, float& Min, float& Max) // Project all four points of the OBB onto the given axis and return the dotproducts of the two outermost points
		{
			Min = (Points[0].x*Axis.x + Points[0].y*Axis.y);
			Max = Min;
			for (int j = 1; j<4; j++)
			{
				float Projection = (Points[j].x*Axis.x + Points[j].y*Axis.y);

				if (Projection<Min)
					Min = Projection;
				if (Projection>Max)
					Max = Projection;
			}
		}
	};

	bool BoundingBoxTest(const sf::Sprite& Object1, const sf::Sprite& Object2) {
		OrientedBoundingBox OBB1(Object1);
		OrientedBoundingBox OBB2(Object2);

		// Create the four distinct axes that are perpendicular to the edges of the two rectangles
		sf::Vector2f Axes[4] = {
			sf::Vector2f(OBB1.Points[1].x - OBB1.Points[0].x,
			OBB1.Points[1].y - OBB1.Points[0].y),
			sf::Vector2f(OBB1.Points[1].x - OBB1.Points[2].x,
			OBB1.Points[1].y - OBB1.Points[2].y),
			sf::Vector2f(OBB2.Points[0].x - OBB2.Points[3].x,
			OBB2.Points[0].y - OBB2.Points[3].y),
			sf::Vector2f(OBB2.Points[0].x - OBB2.Points[1].x,
			OBB2.Points[0].y - OBB2.Points[1].y)
		};

		for (int i = 0; i<4; i++) // For each axis...
		{
			float MinOBB1, MaxOBB1, MinOBB2, MaxOBB2;

			// ... project the points of both OBBs onto the axis ...
			OBB1.ProjectOntoAxis(Axes[i], MinOBB1, MaxOBB1);
			OBB2.ProjectOntoAxis(Axes[i], MinOBB2, MaxOBB2);

			// ... and check whether the outermost projected points of both OBBs overlap.
			// If this is not the case, the Separating Axis Theorem states that there can be no collision between the rectangles
			if (!((MinOBB2 <= MaxOBB1) && (MaxOBB2 >= MinOBB1)))
				return false;
		}
		return true;
	}
}
