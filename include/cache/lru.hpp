#ifndef INCLUDE_LFU_HPP
#define INCLUDE_LFU_HPP

#include <cstddef>
#include <list>
#include <unordered_map>
#include <tuple>

namespace yarvs
{

template<typename KeyT, typename PageT, typename PageGetter>
class LRU final
{
public:

    using key_type = KeyT;
    using page_type = PageT;
    using size_type = std::size_t;
    using page_getter_type = PageGetter;

    LRU(size_type capacity, page_getter_type slow_get_page)
        : hash_table_(capacity), capacity_and_page_getter_{capacity, slow_get_page} {}

    size_type capacity() const noexcept { return std::get<0>(capacity_and_page_getter_); }
    page_getter_type page_getter() const noexcept { return std::get<1>(capacity_and_page_getter_); }

    size_type size() const noexcept { return hash_table_.size(); }
    bool is_full() const noexcept { return size() == capacity(); }

    const page_type &lookup_update(const key_type &key)
    {
        if (auto hit = hash_table_.find(key); hit == hash_table_.end())
        {
            if (is_full())
            {
                hash_table_.erase(pages_.back().first);
                pages_.pop_back();
            }

            pages_.emplace_front(key, page_getter()(key));
            auto page_it = pages_.begin();
            hash_table_.emplace(key, page_it);

            return page_it->second;
        }
        else
        {
            auto page_it = hit->second;

            // [list.ops]: The result is unchanged if position == i
            // That's why we do not need to check that page_it != pages_.begin()
            pages_.splice(pages_.begin(), pages_, page_it);

            return page_it->second;
        }
    }

    void clear()
    {
        pages_.clear();
        hash_table_.clear();
        hash_table_.reserve(capacity());
    }

private:

    std::list<std::pair<key_type, page_type>> pages_;

    using page_iterator = decltype(pages_)::iterator;

    std::unordered_map<key_type, page_iterator> hash_table_;
    std::tuple<size_type, page_getter_type> capacity_and_page_getter_; // for EBO purposes
};

} // namespace yarvs

#endif // INCLUDE_LFU_HPP
