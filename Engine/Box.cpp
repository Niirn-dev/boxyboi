#include "Box.h"
#include "ColorTrait.h"
#include "Rect.h"
IndexedTriangleList<Vec2> Box::model;

std::unique_ptr<Box> Box::Spawn( float size,const Boundaries& bounds,b2World& world,std::mt19937& rng )
{
	std::uniform_real_distribution<float> pos_dist(
		-bounds.GetSize() + size * 2.0f,
		bounds.GetSize() - size * 2.0f
	);
	std::uniform_real_distribution<float> power_dist( 0.0f,6.0f );
	std::uniform_real_distribution<float> angle_dist( -PI,PI );
	std::uniform_int_distribution<int> type_dist( 0,5 );

	const auto linVel = (Vec2{ 1.0f,0.0f } * Mat2::Rotation( angle_dist( rng ) )) * power_dist( rng );
	const auto pos = Vec2{ pos_dist( rng ),pos_dist( rng ) };
	const auto ang = angle_dist( rng );
	const auto angVel = angle_dist( rng ) * 1.5f;

	std::unique_ptr<ColorTrait> pColorTrait;
	switch( type_dist( rng ) )
	{
	case 0:
		pColorTrait = std::make_unique<RedTrait>();
		break;
	case 1:
		pColorTrait = std::make_unique<GreenTrait>();
		break;
	case 2:
		pColorTrait = std::make_unique<BlueTrait>();
		break;
	case 3:
		pColorTrait = std::make_unique<WhiteTrait>();
		break;
	case 4:
		pColorTrait = std::make_unique<YellowTrait>();
		break;
	case 5:
		pColorTrait = std::make_unique<CyanTrait>();
		break;
	}
	
	return std::make_unique<Box>( std::move( pColorTrait ),world,pos,size,ang,linVel,angVel );
}

std::unique_ptr<Box> Box::GetExpanded( b2World& world,float delta_size )
{
	// Get pointer to boundries object
	auto body = world.GetBodyList();
	while ( body->GetType() != b2BodyType::b2_staticBody &&
			body != nullptr )
	{
		body = body->GetNext();
	}
	if ( body != nullptr )
	{
		auto bound = reinterpret_cast<Boundaries*>( body->GetUserData() );
		// Add some dead zone around the boundries
		const float boundSize = bound->GetSize() - 0.5f;

		// Make sure box wont go out of bounds when expaned
		Vec2 center = GetPosition();
		const Vec2 baseCorner = Vec2{ size + delta_size,size + delta_size } * Mat2::Rotation( GetAngle() );
		for ( int i = 0; i < 4; ++i )
		{
			const Vec2 corner = GetPosition() + baseCorner * Mat2::Rotation( (float)i * PI / 2.0f );
			// Only want to modify center position once per axis
			if ( center.x == GetPosition().x )
			{
				if ( corner.x > boundSize )
				{
					center.x -= corner.x - boundSize;
				}
				else if ( corner.x < -boundSize )
				{
					center.x -= corner.x + boundSize;
				}
			}
			if ( center.y == GetPosition().y )
			{
				if ( corner.y > boundSize )
				{
					center.y -= corner.y - boundSize;
				}
				else if ( corner.y < -boundSize )
				{
					center.y -= corner.y + boundSize;
				}
			}
		}

		auto expanded = std::make_unique<Box>(
			GetColorTrait().Clone(),
			world,
			center,
			size + delta_size,
			GetAngle(),
			GetVelocity(),
			GetAngularVelocity()
			);
		MarkForDeath();
		return std::move( expanded );
	}

	// Should never get here. In theory
	return nullptr;
}

std::vector<std::unique_ptr<Box>> Box::Split( b2World& world )
{
	std::vector<std::unique_ptr<Box>> boxes;
	const Vec2 pos = GetPosition();
	const float angle = GetAngle();
	const Vec2 vel = GetVelocity();
	const float angVel = GetAngularVelocity();
	// base for rotation to calculate centers of children relative to parent center
	const Vec2 base = ( Vec2{ 0.5f,0.5f } * size ) * Mat2::Rotation( angle );
	for ( int i = 0; i < 4; i++ )
	{
		boxes.push_back( std::make_unique<Box>(
			GetColorTrait().Clone(),world,
			base * Mat2::Rotation( (float)i * PI / 2.0f ) + pos,
			GetSize() / 2.0f,angle,vel,angVel
			) );
	}
	MarkForDeath();
	return boxes;
}
