#pragma once

namespace CirclePhysics
{

// Shader sources
const char* vertexShaderSource = R"(
#version 330 core

layout (location = 0) in vec2 a_Vertex;
layout (location = 1) in vec2 a_Offset;
layout (location = 2) in vec3 a_Color;
layout (location = 3) in float a_Radius;
layout (location = 4) in float a_OutlineWidth;

out vec2 v_Fragment;
out vec3 v_Color;
out float v_Radius;
out float v_OutlineWidth;

uniform mat4 u_Projection;

void main()
{
    vec2 position = a_Offset + a_Vertex * a_Radius;
    gl_Position = u_Projection * vec4(position, 0.0, 1.0);
    v_Fragment = a_Vertex;
    v_Color = a_Color;
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