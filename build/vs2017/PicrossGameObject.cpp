#include "PicrossGameObject.h"

PicrossGameObject::PicrossGameObject() : gef::MeshInstance()
{
    velocity = gef::Vector4::kZero;
    position = gef::Vector4::kZero;
    rotation = gef::Vector4::kZero;
}

PicrossGameObject::~PicrossGameObject()
{
}

void PicrossGameObject::setRotation(gef::Vector4 rotation)
{
    this->rotation = rotation;
    updateTransform();
}

gef::Vector4 PicrossGameObject::getRotation()
{
    return rotation;
}

void PicrossGameObject::setPosition(gef::Vector4 position)
{
    this->position = position;
    updateTransform();
}

gef::Vector4 PicrossGameObject::getPosition()
{
    return position;
}

gef::Quaternion PicrossGameObject::createQuaternion()
{
    float yaw = rotation.z();
    float pitch = rotation.y();
    float roll = rotation.x();

    //Source from https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    double cy = cos(yaw * 0.5);
    double sy = sin(yaw * 0.5);
    double cp = cos(pitch * 0.5);
    double sp = sin(pitch * 0.5);
    double cr = cos(roll * 0.5);
    double sr = sin(roll * 0.5);

    gef::Quaternion q;
    q.w = cr * cp * cy + sr * sp * sy;
    q.x = sr * cp * cy - cr * sp * sy;
    q.y = cr * sp * cy + sr * cp * sy;
    q.z = cr * cp * sy - sr * sp * cy;

    return q;
}

void PicrossGameObject::updateTransform()
{
    gef::Matrix44 transform, translation, rotation;
    transform.SetIdentity(); translation.SetIdentity(); rotation.SetIdentity();

	translation.SetTranslation(position);
    rotation.Rotation(createQuaternion());
	
	transform = rotation * translation;

	set_transform(transform);
}
