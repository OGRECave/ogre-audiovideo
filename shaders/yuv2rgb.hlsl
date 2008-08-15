sampler diffuseMap;

float4 ps_main(float2 texCoord : TEXCOORD0) : COLOR0
{   
   float3 yuv=tex2D(diffuseMap,texCoord).xyz;
   float y,u,v,r,g,b;
   
   y=1.1643*(yuv.x-0.0625);
   u=yuv.y-0.5;
   v=yuv.z-0.5;
   
   r=y+1.5958*v;
   g=y-0.39173*u-0.81290*v;
   b=y+2.017*u;


   return float4(r,g,b,1.0f);
   
}
