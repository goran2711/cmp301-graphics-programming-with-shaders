// Geometry shader for updating particle system

#include "particle_header.hlsl"

float3 RandVec3(float offset)
{
	// Calculate a random texture coordinate (index)
	float u = gTotalTime + offset;

	// Sample the random texture
	float3 v = gRandomTexture.SampleLevel(gLinearSampler, u, 0).xyz;

	return v;
}

float3 RandUnitVec3(float offset)
{
	// Normalise the random vector
	return normalize(RandVec3(offset));
}

// Max the number of emitted particles + the emitter itself
[maxvertexcount(EMIT_NUM + 1)]
void main(point Particle geoIn[1], inout PointStream<Particle> geoOut)
{
	geoIn[0].age += gFrameTime;

	// If this is a normal particle (flare)
	if (geoIn[0].type != EMITTER)
	{
		// If it has not "died"
		if (geoIn[0].age < MAX_AGE)
			// Keep drawing it
			geoOut.Append(geoIn[0]);
		
		return;
	}

	// If it is time to spawn another particle
	if (geoIn[0].age >= EMIT_INTERVAL)
	{
		// Emit a predetermined number of particles
		for (int i = 0; i < EMIT_NUM; ++i)
		{
			// Get a random vector to act as an offset
			float3 vRandom = EMIT_RADIUS * RandUnitVec3((float) i / 5.f);
			vRandom.y = EMIT_HEIGHT;

			// Create a new particle
			Particle p;
			p.initialPositionW = gEmitPositionW + vRandom;
			p.initialVelocityW = float3(WIND_INTENSITY, -RAINDROP_FORCE, 0.f);
			p.sizeW = 1.f;
			p.age = 0.f;
			p.type = FLARE;

			geoOut.Append(p);
		}

		// Reset the emission timer
		geoIn[0].age = 0.f;
	}

	// Never delete emitters
	geoOut.Append(geoIn[0]);
}