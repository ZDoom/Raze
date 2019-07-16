#ifndef linklist_h_
#define linklist_h_

#include "compat.h"

namespace LL
{
template <typename T> FORCE_INLINE CONSTEXPR_CXX14 void Reset(T const node) { node->next = node->prev = node; }
template <typename T> FORCE_INLINE CONSTEXPR_CXX14 void Unlink(T const node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

template <typename T> FORCE_INLINE CONSTEXPR_CXX14 void Insert(T const root, T const node)
{
    node->next       = root;
    node->prev       = root->prev;
    root->prev->next = node;
    root->prev       = node;
}

template <typename T> FORCE_INLINE CONSTEXPR_CXX14 void Remove(T const node)
{
    Unlink(node);
    Reset(node);
}

template <typename T> FORCE_INLINE CONSTEXPR_CXX14 void Move(T const node, T const root)
{
    Unlink(node);
    Insert(root, node);
}

template <typename T, typename Tt> FORCE_INLINE CONSTEXPR_CXX14 void SortedInsert(T const root, T const node, Tt remove_pointer_t<T>::*m)
{
    T best = root->next;
    while ((best != root) && (node->*m > best->*m))
        best = best->next;
    Insert(best, node);
}

template <typename T> FORCE_INLINE CONSTEXPR bool Empty(T const root) { return ((root->next == root) && (root->prev == root)); }

#if 0
template <typename T> FORCE_INLINE void ReverseList(T const root)
{
    T node = root->next;
    for (T trav = root->prev, tprev; trav != node; trav = tprev)
    {
        tprev = trav->prev;
        Move(trav, node);
    }
}
#endif
}  // namespace LL
#endif
