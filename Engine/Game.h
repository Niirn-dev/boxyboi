/******************************************************************************************
*	Chili DirectX Framework Version 16.10.01											  *
*	Game.h																				  *
*	Copyright 2016 PlanetChili.net <http://www.planetchili.net>							  *
*																						  *
*	This file is part of The Chili DirectX Framework.									  *
*																						  *
*	The Chili DirectX Framework is free software: you can redistribute it and/or modify	  *
*	it under the terms of the GNU General Public License as published by				  *
*	the Free Software Foundation, either version 3 of the License, or					  *
*	(at your option) any later version.													  *
*																						  *
*	The Chili DirectX Framework is distributed in the hope that it will be useful,		  *
*	but WITHOUT ANY WARRANTY; without even the implied warranty of						  *
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the						  *
*	GNU General Public License for more details.										  *
*																						  *
*	You should have received a copy of the GNU General Public License					  *
*	along with The Chili DirectX Framework.  If not, see <http://www.gnu.org/licenses/>.  *
******************************************************************************************/
#pragma once

#include "Graphics.h"
#include <memory>
#include <vector>
#include "FrameTimer.h"
#include <Box2D\Box2D.h>
#include "Box.h"
#include "Boundaries.h"
#include "Pipeline.h"
#include "SolidEffect.h"
#include <random>
#include <unordered_map>
#include <functional>

class Game
{
private:
	class EventManager
	{
	public:
		EventManager()
		{
			dflt = []() {};
		}
		std::function<void( const std::pair<Box*,Box*>& )>& Case( std::pair<Color,Color> cp )
		{
			return map[cp];
		}
		std::function<void()>& Default()
		{
			return dflt;
		}
		void Call( const std::pair<Box*,Box*>& bp )
		{
			// Look up exact pair
			if ( auto target = map.find( { bp.first->GetColorTrait().GetColor(),bp.second->GetColorTrait().GetColor() } );
				 target != map.end() )
			{
				target->second( bp );
			}
			// Look up reversed pair
			else if ( auto target = map.find( { bp.second->GetColorTrait().GetColor(),bp.first->GetColorTrait().GetColor() } );
					 target != map.end() )
			{
				target->second( bp );
			}
			// Do default
			else
			{
				dflt();
			}
		}

	private:
		std::unordered_map<std::pair<Color,Color>,std::function<void( const std::pair<Box*,Box*>& )>> map;
		std::function<void()> dflt;
	};
public:
	Game( class MainWindow& wnd );
	Game( const Game& ) = delete;
	Game& operator=( const Game& ) = delete;
	void Go();
private:
	void ComposeFrame();
	void UpdateModel();
	/********************************/
	/*  User Functions              */
	bool SplitBox( const Box* box,unsigned int factor = 2 );
	void DestroyBox( const Box* box );
	void SplitSmallest( const std::pair<Box*,Box*>& bp );
	/********************************/
private:
	MainWindow& wnd;
	Graphics gfx;
	/********************************/
	/*  User Variables              */
	static constexpr float boundarySize = 10.0f;
	static constexpr float boxSize = 1.0f;
	static constexpr float minBoxSize = 0.15f;
	static constexpr int nBoxes = 6;
	std::mt19937 rng = std::mt19937( std::random_device{}() );
	FrameTimer ft;
	Pipeline<SolidEffect> pepe;
	b2World world;
	Boundaries bounds = Boundaries( world,boundarySize );
	std::vector<std::unique_ptr<Box>> boxPtrs;
	EventManager em;
	/********************************/
};