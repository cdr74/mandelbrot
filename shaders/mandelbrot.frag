#version 330 core

uniform vec2  u_center;
uniform float u_zoom;
uniform int   u_maxIter;
uniform int   u_colorScheme;
uniform float u_colorOffset;
uniform vec2  u_resolution;

out vec4 FragColor;

// Standard HSV to RGB conversion
vec3 hsv2rgb(float h, float s, float v) {
    h = fract(h);
    float i = floor(h * 6.0);
    float f = h * 6.0 - i;
    float p = v * (1.0 - s);
    float q = v * (1.0 - f * s);
    float t = v * (1.0 - (1.0 - f) * s);
    int sector = int(i) % 6;
    if (sector == 0) return vec3(v, t, p);
    if (sector == 1) return vec3(q, v, p);
    if (sector == 2) return vec3(p, v, t);
    if (sector == 3) return vec3(p, q, v);
    if (sector == 4) return vec3(t, p, v);
                     return vec3(v, p, q);
}

// 5-stop gradient helper
vec3 gradient5(float t, vec3 c0, vec3 c1, vec3 c2, vec3 c3, vec3 c4) {
    t = clamp(t, 0.0, 1.0) * 4.0;
    int s = int(t);
    float f = fract(t);
    if (s == 0) return mix(c0, c1, f);
    if (s == 1) return mix(c1, c2, f);
    if (s == 2) return mix(c2, c3, f);
                return mix(c3, c4, f);
}

vec3 applyColorScheme(float smooth_i) {
    // Color scheme 0: Classic — HSV hue cycling
    if (u_colorScheme == 0) {
        float t = fract(smooth_i * 0.015);
        return hsv2rgb(t, 0.85, 1.0);
    }
    // Color scheme 1: Fire — black→red→orange→yellow→white
    if (u_colorScheme == 1) {
        float t = fract(smooth_i * 0.02);
        return gradient5(t,
            vec3(0.0, 0.0, 0.0),
            vec3(0.5, 0.0, 0.0),
            vec3(1.0, 0.3, 0.0),
            vec3(1.0, 0.8, 0.0),
            vec3(1.0, 1.0, 1.0));
    }
    // Color scheme 2: Ocean — black→navy→cyan→white
    if (u_colorScheme == 2) {
        float t = fract(smooth_i * 0.02);
        return gradient5(t,
            vec3(0.0,  0.0,  0.0),
            vec3(0.0,  0.05, 0.3),
            vec3(0.0,  0.4,  0.8),
            vec3(0.0,  0.9,  1.0),
            vec3(1.0,  1.0,  1.0));
    }
    // Color scheme 3: Grayscale
    if (u_colorScheme == 3) {
        float t = fract(smooth_i * 0.02);
        float v = sqrt(t);
        return vec3(v, v, v);
    }
    // Color scheme 4: Psychedelic — high-freq HSV with phase shift
    if (u_colorScheme == 4) {
        float t = smooth_i * 0.07;
        float h = fract(t);
        float s = 0.5 + 0.5 * sin(t * 3.14159);
        return hsv2rgb(h, s, 1.0);
    }
    // Color scheme 5: Ultra — black→purple→blue→gold→white
    // (default / fallthrough)
    {
        float t = fract(smooth_i * 0.012);
        return gradient5(t,
            vec3(0.0,  0.0,  0.0),
            vec3(0.3,  0.0,  0.5),
            vec3(0.0,  0.2,  0.9),
            vec3(0.9,  0.7,  0.0),
            vec3(1.0,  1.0,  1.0));
    }
}

void main() {
    float aspect = u_resolution.x / u_resolution.y;

    // gl_FragCoord origin is bottom-left; map to [-0.5, 0.5]
    vec2 uv = gl_FragCoord.xy / u_resolution - 0.5;
    uv.y = -uv.y; // flip Y so +imaginary is up

    vec2 c = u_center + uv * vec2(2.0 * u_zoom * aspect, 2.0 * u_zoom);

    vec2 z = vec2(0.0);
    int i;
    for (i = 0; i < u_maxIter; i++) {
        if (dot(z, z) > 4.0) break;
        z = vec2(z.x * z.x - z.y * z.y + c.x,
                 2.0 * z.x * z.y + c.y);
    }

    // Interior: black
    if (i == u_maxIter) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    // Smooth iteration count (Inigo Quilez formulation)
    float smooth_i = float(i) - log2(log2(dot(z, z))) + 1.0;

    FragColor = vec4(applyColorScheme(smooth_i + u_colorOffset), 1.0);
}
