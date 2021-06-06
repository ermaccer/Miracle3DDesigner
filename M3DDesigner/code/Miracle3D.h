#pragma once
#include <vector>

// static
#define MODEL_INFO_IDENT   2
#define VERTICES_IDENT     3	
#define FACES_IDENT        61441
#define END_INFO_IDENT     61442

// sam
#define BONE_INFO_IDENT    3
#define GROUP_INFO_IDENT   5
#define ANIM_INFO_IDENT    61441	
#define ANIM_ENTRIES	   13


enum eMiracle3DModelType {
	M3D_MODEL_STATIC = 5,
	M3D_MODEL_ANIMATED
};

enum eGroupTypes {
	GROUP_STATIC = 1,
	GROUP_ANIMATED,
	GROUP_DUMMY
};

struct vector3d {
	float x, y, z;
};

struct quaternion3d {
	float x, y, z, w;
};


struct m3d_header {
	char header[8] = {};
	int  type;
	int  version;
};

struct m3d_section {
	int size;
	int offset;
	int _pad;
	int type;
};


struct model_info {
	short boneAmount;
	short actualBones;
	short dummyObjects;
	short normalObjects;
	short skinnedObjects;
	short animations;
	char  textureName[40] = {};
};


struct static_model_header {
	int    header;
	int    type;
	int    vertices;
	int    faces;
	char   unk;
	char   textName[131] = {};
};


struct bone_info {
	char boneName[40] = {};
	short boneID;
	short parentID;
	vector3d pos;
	quaternion3d rot;
};

#pragma pack(push,1)
struct bone_link {
	short boneID;
	float weight;
};
#pragma (pop)

struct group_info {
	char name[40] = {};
	short verts;
	short faces;
	short unk;
	short type;
};

#pragma pack(push,1)
struct anim_header {
	short  bones;
	short  frames;
	short  framerate;
	short  unkAmount;
	short  animID;
	char   name[24] = {};
	char   pad;

};
#pragma (pop)

struct anim_frame {
	short  boneID;
	vector3d pos;
	quaternion3d rot;
};


struct end_info {
	int header; // SAM \0 or GSM \0
	char pad[26] = {};
};


struct v { float x, y, z; };
struct uv { float u, v; };
struct vn { float norm[3]; };
struct face { short face[3]; };

struct v_data {
	v vert;
	std::vector<bone_link> BoneLinks;
};


#include <math.h>
#define M_PI           3.14159265358979323846f

#define degToRad(angleInDegrees) ((angleInDegrees) * M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / M_PI)

vector3d		quat2vec(quaternion3d q);
quaternion3d	vec2quat(vector3d v);

bool			areVecsEqual(vector3d v1, vector3d v2);
bool			areVertsEqual(v_data v1, v_data v2);

