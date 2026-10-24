#version 440
layout(location = 0) in vec2 qt_TexCoord0;
layout(location = 0) out vec4 fragColor;
layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    vec2 controlSize;
    vec4 glassRect;
    vec4 accentColor;
    float lensStrength;
};
layout(binding = 1) uniform sampler2D source;

float capsuleDistance(vec2 point, vec2 center, vec2 halfSize) {
    float radius = halfSize.y;
    vec2 q = abs(point - center) - (halfSize - vec2(radius));
    return length(max(q, vec2(0.0))) + min(max(q.x, q.y), 0.0) - radius;
}
vec4 over(vec4 foreground, vec4 background) {
    return foreground + background * (1.0 - foreground.a);
}
void main() {
    vec2 p = qt_TexCoord0 * controlSize;
    vec2 center = glassRect.xy + glassRect.zw * 0.5;
    vec2 halfSize = glassRect.zw * 0.5;
    float radius = halfSize.y;
    float distance = capsuleDistance(p, center, halfSize);
    float coverage = 1.0 - smoothstep(-0.65, 0.65, distance);

    // A convex rim pulls the source coordinates inward, magnifying text
    // as either curved edge passes across a glyph.
    vec2 local = p - center;
    vec2 nearest = vec2(clamp(local.x, -halfSize.x + radius, halfSize.x - radius), 0.0);
    vec2 radial = local - nearest;
    vec2 normal = radial / max(length(radial), 0.001);
    float rim = exp(-pow((distance + 5.5) / 8.5, 2.0)) * coverage;
    // Only the moving curved side rims refract strongly. The horizontal
    // top/bottom borders must not stretch a centered, stationary label.
    rim *= smoothstep(0.08, 0.55, abs(normal.x));
    vec2 samplePoint = p - normal * lensStrength * rim - local * (0.028 * coverage);
    vec2 uv = clamp(samplePoint / controlSize, vec2(0.0), vec2(1.0));
    vec4 plainText = texture(source, qt_TexCoord0);
    vec4 bentText = texture(source, uv);
    vec4 textColor = mix(plainText, bentText, coverage);
    textColor.rgb = mix(textColor.rgb, accentColor.rgb * textColor.a, coverage * 0.96);

    float shadowDistance = capsuleDistance(p - vec2(0.0, 2.1), center, halfSize);
    float shadowAlpha = 0.16 * exp(-pow(max(shadowDistance, 0.0) / 3.2, 2.0));
    vec4 shadow = vec4(vec3(0.40, 0.28, 0.20) * shadowAlpha, shadowAlpha);

    float heightFraction = clamp((p.y - glassRect.y) / glassRect.w, 0.0, 1.0);
    vec3 material = mix(vec3(1.0, 0.976, 0.947), vec3(0.966, 0.856, 0.788), heightFraction);
    float gloss = 0.37 * exp(-pow((heightFraction - 0.13) / 0.20, 2.0));
    material = mix(material, vec3(1.0), gloss);
    material = mix(material, accentColor.rgb, 0.025);
    float glassAlpha = coverage * 0.82;
    vec4 glass = vec4(material * glassAlpha, glassAlpha);

    float rimStroke = (1.0 - smoothstep(0.3, 1.25, abs(distance + 0.45))) * coverage;
    float lightSide = mix(0.43, 0.96, 1.0 - heightFraction);
    float rimAlpha = rimStroke * lightSide;
    vec3 rimColor = mix(vec3(0.80, 0.59, 0.46), vec3(1.0), 1.0 - heightFraction);
    vec4 edge = vec4(rimColor * rimAlpha, rimAlpha);
    vec4 surface = over(edge, over(glass, shadow));
    fragColor = over(textColor, surface) * qt_Opacity;
}
