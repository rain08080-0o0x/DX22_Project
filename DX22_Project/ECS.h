#pragma once

#include <cstdint>
#include <memory>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <utility>

namespace ECS
{
    using Entity = uint32_t;
    constexpr Entity kInvalidEntity = 0;

    class World
    {
    public:
        Entity CreateEntity()
        {
            return m_next++;
        }

        void DestroyEntity(Entity entity)
        {
            for (auto& entry : m_stores)
            {
                entry.second->Remove(entity);
            }
        }

        void Clear()
        {
            m_stores.clear();
            m_next = kInvalidEntity + 1;
        }

        template <typename T, typename... Args>
        T& Add(Entity entity, Args&&... args)
        {
            Store<T>& store = GetOrCreateStore<T>();
            T value(std::forward<Args>(args)...);
            auto it = store.data.find(entity);
            if (it == store.data.end())
            {
                auto result = store.data.emplace(entity, std::move(value));
                return result.first->second;
            }

            it->second = std::move(value);
            return it->second;
        }

        template <typename T>
        bool Has(Entity entity) const
        {
            const Store<T>* store = FindStore<T>();
            if (!store)
            {
                return false;
            }

            return store->data.find(entity) != store->data.end();
        }

        template <typename T>
        T* TryGet(Entity entity)
        {
            Store<T>* store = FindStore<T>();
            if (!store)
            {
                return nullptr;
            }

            auto it = store->data.find(entity);
            if (it == store->data.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        template <typename T>
        const T* TryGet(Entity entity) const
        {
            const Store<T>* store = FindStore<T>();
            if (!store)
            {
                return nullptr;
            }

            auto it = store->data.find(entity);
            if (it == store->data.end())
            {
                return nullptr;
            }

            return &it->second;
        }

        template <typename T, typename... Ts, typename Func>
        void Each(Func&& func)
        {
            Store<T>* store = FindStore<T>();
            if (!store)
            {
                return;
            }

            for (auto& entry : store->data)
            {
                const Entity entity = entry.first;
                if constexpr (sizeof...(Ts) == 0)
                {
                    func(entity, entry.second);
                }
                else
                {
                    if ((Has<Ts>(entity) && ...))
                    {
                        func(entity, entry.second, *TryGet<Ts>(entity)...);
                    }
                }
            }
        }

    private:
        struct IStore
        {
            virtual ~IStore() = default;
            virtual void Remove(Entity entity) = 0;
        };

        template <typename T>
        struct Store : IStore
        {
            std::unordered_map<Entity, T> data;
            void Remove(Entity entity) override { data.erase(entity); }
        };

        template <typename T>
        Store<T>* FindStore()
        {
            auto it = m_stores.find(std::type_index(typeid(T)));
            if (it == m_stores.end())
            {
                return nullptr;
            }

            return static_cast<Store<T>*>(it->second.get());
        }

        template <typename T>
        const Store<T>* FindStore() const
        {
            auto it = m_stores.find(std::type_index(typeid(T)));
            if (it == m_stores.end())
            {
                return nullptr;
            }

            return static_cast<const Store<T>*>(it->second.get());
        }

        template <typename T>
        Store<T>& GetOrCreateStore()
        {
            auto& entry = m_stores[std::type_index(typeid(T))];
            if (!entry)
            {
                entry = std::make_unique<Store<T>>();
            }

            return *static_cast<Store<T>*>(entry.get());
        }

        std::unordered_map<std::type_index, std::unique_ptr<IStore>> m_stores;
        Entity m_next = kInvalidEntity + 1;
    };
}
