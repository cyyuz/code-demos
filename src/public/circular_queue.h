/****************************************************************************************/
/*   程序名：circular_queue.h，此程序是公共函数和类的声明文件。                                   */
/*   作者：吴从周                                                                        */
/****************************************************************************************/
#ifndef _CIRCULAR_QUEUE_H
#define _CIRCULAR_QUEUE_H

template <class TT, int MaxLength>
class circular_queue
{
private:
    bool m_inited;              // 队列被初始化标志，true-已初始化；false-未初始化。
    TT   m_data[MaxLength];     // 用数组存储循环队列中的元素。
    int  m_head;                // 队列的头指针。
    int  m_tail;                // 队列的尾指针，指向队尾元素。
    int  m_length;              // 队列的实际长度。    
    circular_queue(const circular_queue &) = delete;             // 禁用拷贝构造函数。
    circular_queue &operator=(const circular_queue &) = delete;  // 禁用赋值函数。
public:

    circular_queue() { init(); }  // 构造函数。

    // 循环队列的初始化操作。
    // 注意：如果用于共享内存的队列，不会调用构造函数，必须调用此函数初始化。
    void init()  
    { 
        if (m_inited!=true)               // 循环队列的初始化只能执行一次。
        { 
            m_head=0;                      // 头指针。
            m_tail=MaxLength-1;     // 为了方便写代码，初始化时，尾指针指向队列的最后一个位置。
            m_length=0;                   // 队列的实际长度。
            memset(m_data,0,sizeof(m_data));  // 数组元素清零。
            m_inited=true; 
        }
    }

    // 元素入队，返回值：false-失败；true-成功。
    bool push(const TT &ee)
    {
        if (full() == true)
        {
            cout << "循环队列已满，入队失败。\n"; return false;
        }

        // 先移动队尾指针，然后再拷贝数据。
        m_tail=(m_tail+1)%MaxLength;  // 队尾指针后移。
        m_data[m_tail]=ee;
        m_length++;    

      return true;
    }

    // 求循环队列的长度，返回值：>=0-队列中元素的个数。
    int  size()                   
    {
        return m_length;    
    }

    // 判断循环队列是否为空，返回值：true-空，false-非空。
    bool empty()                    
    {
      if (m_length == 0) return true;    

      return false;
    }

    // 判断循环队列是否已满，返回值：true-已满，false-未满。
    bool full()
    {
        if (m_length == MaxLength) return true;    

      return false;
    }

    // 查看队头元素的值，元素不出队。
    TT& front()
    {
        return m_data[m_head];
    }

    // 元素出队，返回值：false-失败；true-成功。
    bool pop()
    {
        if (empty() == true) return false;

        m_head=(m_head+1)%MaxLength;  // 队列头指针后移。
        m_length--;    

        return true;
    }

    // 显示循环队列中全部的元素。
    // 这是一个临时的用于调试的函数，队列中元素的数据类型支持cout输出才可用。
    void printqueue()                    
    {
        for (int ii = 0; ii < size(); ii++)
        {
            cout << "m_data[" << (m_head+ii)%MaxLength << "],value=" \
                 << m_data[(m_head+ii)%MaxLength] << endl;
        }
    }
};

#endif // CIRCULAR_QUEUE_H