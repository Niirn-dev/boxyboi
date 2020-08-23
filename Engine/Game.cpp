/******************************************************************************************
*	Chili DirectX Framework Version 16.10.01											  *
*	Game.cpp																			  *
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
#include "MainWindow.h"
#include "Game.h"
#include "Box.h"
#include "Mat2.h"
#include "PatternMatchingListener.h"
#include "ColorTrait.h"
#include <algorithm>
#include <sstream>
#include <typeinfo>
#include <functional>
#include <iterator>
#include <set>

Game::Game( MainWindow& wnd )
	:
	wnd( wnd ),
	gfx( wnd ),
	world( { 0.0f,-0.5f } ),
	pepe( gfx )
{
	pepe.effect.vs.cam.SetPos( { 0.0,0.0f } );
	pepe.effect.vs.cam.SetZoom( 1.0f / boundarySize );

	std::generate_n( std::back_inserter( boxPtrs ),nBoxes,
					 [this]()
					 {
						 return Box::Spawn( boxSize,bounds,world,rng );
					 } );

	static PatternMatchingListener mrLister;
	
	mrLister.Case<BlueTrait,GreenTrait>( [&]( Box* a,Box* b )
										 {
											 if ( a->GetSize() > b->GetSize() )
											 {
												 actions.push_back( std::make_unique<Split>( a ) );
											 }
											 else
											 {
												 actions.push_back( std::make_unique<Split>( b ) );
											 }
										 } );
	mrLister.Case<BlueTrait,WhiteTrait>( [&]( Box* a,Box* b )
										 {
											 if ( a->GetColorTrait().GetColor() == Colors::White )
											 {
												 actions.push_back( std::make_unique<Tag>( a,b->GetColorTrait().Clone() ) );
											 }
											 else
											 {
												 actions.push_back( std::make_unique<Tag>( b,a->GetColorTrait().Clone() ) );
											 }
										 } );
	mrLister.Case<YellowTrait,BlueTrait>( [&]( Box* a,Box* b )
										  {
											  if ( a->GetColorTrait().GetColor() == Colors::Blue )
											  {
												  actions.push_back( std::make_unique<Destroy>( a ) );
											  }
											  else
											  {
												  actions.push_back( std::make_unique<Destroy>( b ) );
											  }
										  } );
	mrLister.Case<RedTrait,YellowTrait>( [&]( Box* a,Box* b )
										 {
											 actions.push_back( std::make_unique<Push>( a ) );
											 actions.push_back( std::make_unique<Push>( b ) );
										 } );
	mrLister.Case<CyanTrait,CyanTrait>( [&]( Box* a,Box* b )
										{
											actions.push_back( std::make_unique<ReverseGravity>() );
										} );

	world.SetContactListener( &mrLister );
}

void Game::Go()
{
	gfx.BeginFrame();
	UpdateModel();
	ComposeFrame();
	gfx.EndFrame();
}

void Game::UpdateModel()
{
	const float dt = ft.Mark();
	world.Step( dt,8,3 );

	if ( !world.IsLocked() )
	{
		// Do actions
		for ( auto& a : actions )
		{
			a->Do( boxPtrs,world );
		}
		actions.clear();

		// Destroy boxes marked for death
		boxPtrs.erase(
			std::remove_if( boxPtrs.begin(),boxPtrs.end(),std::mem_fn( &Box::IsMarkedForDeath ) ),
			boxPtrs.end()
		);
	}
}

void Game::ComposeFrame()
{
	for( const auto& p : boxPtrs )
	{
		p->Draw( pepe );
	}
}