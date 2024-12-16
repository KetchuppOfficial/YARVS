#ifndef INCLUDE_LFU_HPP
#define INCLUDE_LFU_HPP

#include <cstddef>
#include <list>
#include <unordered_map>
#include <cassert>
#include <utility>

namespace yarvs
{

template<typename KeyT, typename PageT>
class LRU final
{
public:

    using key_type = KeyT;
    using page_type = PageT;
    using size_type = std::size_t;
    using page_iterator = typename std::list<std::pair<key_type, page_type>>::const_iterator;

    explicit LRU(size_type capacity) : hash_table_(capacity), capacity_{capacity} {}

    size_type capacity() const noexcept { return capacity_; }
    size_type size() const noexcept { return hash_table_.size(); }
    bool is_full() const noexcept { return size() == capacity(); }

    page_iterator begin() const noexcept { return pages_.begin(); }
    page_iterator end() const noexcept { return pages_.end(); }

    page_iterator lookup(const key_type &key)
    {
        auto hit = hash_table_.find(key);
        if (hit == hash_table_.end())
            return end();

        auto page_it = hit->second;

        // [list.ops]: The result is unchanged if position == i
        // That's why we do not need to check that page_it != pages_.begin()
        pages_.splice(pages_.begin(), pages_, page_it);

        return page_it;
    }

    void update(const key_type &key, const page_type &page) { update_impl(key, page); }
    void update(const key_type &key, page_type &&page) { update_impl(key, std::move(page)); }

    void clear()
    {
        pages_.clear();
        hash_table_.clear();
        hash_table_.reserve(capacity());
    }

private:

    template<typename P>
    void update_impl(const key_type &key, P &&page)
    {
        assert(!hash_table_.contains(key));

        if (is_full())
        {
            hash_table_.erase(pages_.back().first);
            pages_.pop_back();
        }

        pages_.emplace_front(key, std::forward<P>(page));
        hash_table_.emplace(key, pages_.begin());
    }

    std::list<std::pair<key_type, page_type>> pages_;
    std::unordered_map<key_type, page_iterator> hash_table_;
    size_type capacity_;
};

} // namespace yarvs

#endif // INCLUDE_LFU_HPP
