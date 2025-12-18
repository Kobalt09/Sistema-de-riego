"use client";

import { Canvas, useFrame, useThree } from "@react-three/fiber";
import { useMemo, useRef, useEffect } from "react";
import * as THREE from "three";

const vertexShader = `
varying vec2 vUv;
void main() {
  vUv = uv;
  gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
}
`;

// Adapted from user's shadertoy request
const fragmentShader = `
uniform float uTime;
uniform vec2 uResolution;
varying vec2 vUv;

#define time uTime
#define resolution uResolution
#define PI 3.14159265

// Helper to simulate texture noise since we don't have iChannel0 handy in the same way
// We will generate static noise
float hash12(vec2 p) {
  vec3 p3  = fract(vec3(p.xyx) * .1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

vec3 tex2D_mock(vec2 _p) {
  // Simulate the "image" content being just white color with some noise
  // This essentially makes the distortion visible on "white" pixels
  // We want the glitches to result in colors on top of transparent
  float n = hash12(_p * time * 0.1); 
  return vec3(0.0, 0.0, 0.0); // Base is transparent/black
}

float hash( vec2 _v ){
  return fract( sin( dot( _v, vec2( 89.44, 19.36 ) ) ) * 22189.22 );
}

float iHash( vec2 _v, vec2 _r ){
  float h00 = hash( vec2( floor( _v * _r + vec2( 0.0, 0.0 ) ) / _r ) );
  float h10 = hash( vec2( floor( _v * _r + vec2( 1.0, 0.0 ) ) / _r ) );
  float h01 = hash( vec2( floor( _v * _r + vec2( 0.0, 1.0 ) ) / _r ) );
  float h11 = hash( vec2( floor( _v * _r + vec2( 1.0, 1.0 ) ) / _r ) );
  vec2 ip = vec2( smoothstep( vec2( 0.0, 0.0 ), vec2( 1.0, 1.0 ), mod( _v*_r, 1. ) ) );
  return ( h00 * ( 1. - ip.x ) + h10 * ip.x ) * ( 1. - ip.y ) + ( h01 * ( 1. - ip.x ) + h11 * ip.x ) * ip.y;
}

float noise( vec2 _v ){
  float sum = 0.;
  for( int i=1; i<9; i++ )
  {
    sum += iHash( _v + vec2( i ), vec2( 2. * pow( 2., float( i ) ) ) ) / pow( 2., float( i ) );
  }
  return sum;
}

void main() {
  vec2 uv = vUv;
  vec2 uvn = uv;
  vec3 col = vec3( 0.0 );

  // tape wave
  uvn.x += ( noise( vec2( uvn.y, time ) ) - 0.5 )* 0.005;
  uvn.x += ( noise( vec2( uvn.y * 100.0, time * 10.0 ) ) - 0.5 ) * 0.01;

  // tape crease
  float tcPhase = clamp( ( sin( uvn.y * 8.0 - time * PI * 1.2 ) - 0.92 ) * noise( vec2( time ) ), 0.0, 0.01 ) * 10.0;
  float tcNoise = max( noise( vec2( uvn.y * 100.0, time * 10.0 ) ) - 0.5, 0.0 );
  uvn.x = uvn.x - tcNoise * tcPhase;

  // switching noise
  float snPhase = smoothstep( 0.03, 0.0, uvn.y );
  uvn.y += snPhase * 0.3;
  uvn.x += snPhase * ( ( noise( vec2( uv.y * 100.0, time * 10.0 ) ) - 0.5 ) * 0.2 );
    
  // Since we don't have an input channel (Video/Image) to distort, 
  // we will visualize the distortion itself using a solid color where distortion happens
  // OR simulate "static" on the screen.
  
  // We'll generate "static" on the distorted UVs
  float staticNoise = hash12(uvn * resolution.y + time);
  
  // Base color: mostly transparent, but add static
  col = vec3(staticNoise * 0.1); 

  // Make the switching noise visible as white bars
  if (snPhase > 0.0) {
      col += vec3(snPhase * 0.2);
  }
  
  // Crease noise -> white lines preferably
  if (tcPhase > 0.0) {
      col += vec3(tcPhase);
  }

  // Bloom-ish smear (simulated by just adding noise offset samples)
  // For overlay, we'll keep it simple: just output the noise/glitch
  col *= 1.0 + clamp( noise( vec2( 0.0, uv.y + time * 0.2 ) ) * 0.6 - 0.25, 0.0, 0.1 );

  // Output with transparency
  // We want to overlay this. If it's black (0,0,0), alpha should be low.
  // If it has noise (glitch), alpha should be visible.
  float alpha = length(col);
  
  // Boost alpha for visibility of effects
  if (alpha > 0.01) alpha = 0.15; // faint static
  if (tcPhase > 0.1) alpha = 0.5; // strong glitch
  
  gl_FragColor = vec4( col, alpha );
}
`;

const Screen = () => {
  const meshRef = useRef<THREE.Mesh>(null);
  const { size } = useThree();

  const uniforms = useMemo(
    () => ({
      uTime: { value: 0 },
      uResolution: { value: new THREE.Vector2(size.width, size.height) },
    }),
    [] // Create once
  );

  // Update resolution when size changes
  useEffect(() => {
    uniforms.uResolution.value.set(size.width, size.height);
  }, [size, uniforms]);

  useFrame((state) => {
    if (meshRef.current) {
      // @ts-ignore
      meshRef.current.material.uniforms.uTime.value = state.clock.getElapsedTime();
    }
  });

  return (
    <mesh ref={meshRef}>
      <planeGeometry args={[size.width, size.height]} />
      <shaderMaterial
        vertexShader={vertexShader}
        fragmentShader={fragmentShader}
        uniforms={uniforms}
        transparent={true}
        blending={THREE.AdditiveBlending}
      />
    </mesh>
  );
};

export default function VHSOverlay() {
  return (
    <div className="fixed inset-0 pointer-events-none z-50 mix-blend-screen opacity-50">
      <Canvas
        camera={{ position: [0, 0, 1] }}
        orthographic
        gl={{ alpha: true, antialias: false }}
        style={{ pointerEvents: "none" }}
      >
        <Screen />
      </Canvas>
    </div>
  );
}
