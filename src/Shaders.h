#pragma once

namespace CirclePhysics
{

const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 a_Vertex;          // Base quad vertex
layout (location = 1) in float a_PositionX;      // Current position X
layout (location = 2) in float a_PositionY;      // Current position Y
layout (location = 3) in float a_PrevPositionX;  // Previous position X
layout (location = 4) in float a_PrevPositionY;  // Previous position Y
layout (location = 5) in float a_Red;            // Red component
layout (location = 6) in float a_Green;          // Green component
layout (location = 7) in float a_Blue;           // Blue component
layout (location = 8) in float a_Radius;         // Radius
layout (location = 9) in float a_OutlineWidth;   // Outline width

out vec2 v_Fragment;
out vec3 v_Color;
out float v_Radius;
out float v_OutlineWidth;

uniform mat4 u_Projection;
uniform float u_InterpolationFactor; // Alpha for interpolation

void main()
{
    vec2 currentPosition = vec2(a_PositionX, a_PositionY);
    vec2 previousPosition = vec2(a_PrevPositionX, a_PrevPositionY);
    
    // Interpolate between previous and current positions
    vec2 interpolatedPosition = mix(previousPosition, currentPosition, u_InterpolationFactor);
    
    // Apply vertex offset based on radius to create the circle
    vec2 position = interpolatedPosition + a_Vertex * a_Radius;
    
    gl_Position = u_Projection * vec4(position, 0.0, 1.0);
    v_Fragment = a_Vertex;
    v_Color = vec3(a_Red, a_Green, a_Blue);
    v_Radius = a_Radius;
    v_OutlineWidth = a_OutlineWidth;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core

in vec2 v_Fragment;
in vec3 v_Color;
in float v_Radius;
in float v_OutlineWidth;

out vec4 o_FragColor;

uniform bool u_OutlineCircles;

void main()
{
    float distanceToFrag = length(v_Fragment);

    // everything outside of the circle is culled
    if (distanceToFrag > 1.0)
    {
        discard;
    }

    float alpha = 1.0;
    if (u_OutlineCircles)
    {
        if (distanceToFrag < 1.0 - v_OutlineWidth)
        {
            discard;
        }
    }
    else
    {
        // apply antialiasing when circles are filled
        alpha = 1.0 - smoothstep(1.0 - v_OutlineWidth, 1.0, distanceToFrag);    
    }
    
    o_FragColor = vec4(v_Color, alpha);
}
)";

}