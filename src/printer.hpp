#pragma once
#include "headers.hpp"

class printer {
public:
    printer(boost::asio::io_context &io)
    : timer_(io, boost::posix_time::seconds(1)), 
      counter_(0) {
        timer_.async_wait(boost::bind(&printer::print, this,
                boost::asio::placeholders::error));
    }
    void print(const boost::system::error_code& /*e*/)
    {
        if (true)
        {
            std::cout << "Server run time: " << counter_ << std::endl;
            ++(counter_);

            timer_.expires_at(timer_.expires_at() + boost::posix_time::seconds(5));
            timer_.async_wait(boost::bind(&printer::print, this,
                boost::asio::placeholders::error));
        }
    }
private:
    int counter_;
    boost::asio::deadline_timer timer_;
    
};