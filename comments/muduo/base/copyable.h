#ifndef MUDUO_BASE_COPYABLE_H
#define MUDUO_BASE_COPYABLE_H

namespace muduo
{

//һ���ջ��࣬����ʶ�࣬������ʶ���Ǽ̳и���Ķ�����ֵ���ͣ�����ֱ�ӿ���
//��Ϊ ֵ����
/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
};

};

#endif  // MUDUO_BASE_COPYABLE_H
