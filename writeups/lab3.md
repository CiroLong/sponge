Lab 3 Writeup
=============

My name: [your name here]

My SUNet ID: [your sunetid here]

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the TCPSender:
[]

Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]



Start At 2021/11



### It will be your TCPSender’s responsibility to:

+ Keep track of the receiver’s window (processing incoming acknos and window sizes)

  跟踪接收器的窗口（处理传入的确认和窗口大小

+ Fill the window when possible, by reading from the ByteStream, creating new TCP
  segments (including SYN and FIN flags if needed), and sending them. The sender
  should keep sending segments until either the window is full or the ByteStream is
  empty.

  在可能的情况下填充窗口，方法是从 ByteStream 中读取数据，创建新的 TCP 段（如果需要，包括 SYN 和 FIN 标志）并发送它们。发送方应继续发送段，直到窗口已满或 ByteStream 为空

+ Keep track of which segments have been sent but not yet acknowledged by the receiver—
  we call these “outstanding” segments

  跟踪哪些段已发送但尚未被接收方确认——我们称这些段为“未完成”段

+ Re-send outstanding segments if enough time passes since they were sent, and they
  haven’t been acknowledged yet

  如果自发送后经过足够的时间，则重新发送未完成的段，并且它们还没有被确认



### 3.1 How does the TCPSender know if a segment was lost?
> Your TCPSender will be sending a bunch of TCPSegments. Each will contain a (possibly-
> empty) substring from the outgoing ByteStream, indexed with a sequence number to indicate
> its position in the stream, and marked with the SYN flag at the beginning of the stream, and
> FIN flag at the end.

您的 TCPSender 将发送一堆 TCPSegments。每个将包含来自传出 ByteStream 的（可能为空的）子字符串，用序列号索引以指示其在流中的位置，并在流的开头用 SYN 标志进行标记，在末尾用 FIN 标志进行标记。

> In addition to sending those segments, the TCPSender also has to keep track of its outstanding segments until the sequence numbers they occupy have been fully acknowledged. Periodically,
>
> the owner of the TCPSender will call the TCPSender’s tick method, indicating the passage
> of time. The TCPSender is responsible for looking through its collection of outstanding
> TCPSegments and deciding if the oldest-sent segment has been outstanding for too long
> without acknowledgment (that is, without all of its sequence numbers being acknowledged).
> If so, it needs to be retransmitted (sent again).

除了发送这些段之外，TCPSender 还必须跟踪其未完成的段，直到它们占用的序列号被完全确认。 TCPSender 的所有者会定期调用 TCPSender 的 tick 方法，指示时间的流逝。

TCPSender 负责查看其未完成的 TCPSegment 的集合，并确定最旧的发送段是否在没有确认的情况下未完成的时间过长（即，没有确认其所有序列号）。如果是，则需要重传（再次发送）。



> Here are the rules for what “outstanding for too long” means.2 You’re going to be implement-
> ing this logic, and it’s a little detailed, but we don’t want you to be worrying about hidden
> test cases trying to trip you up or treating this like a word problem on the SAT. We’ll give
> you some reasonable unit tests this week, and fuller integration tests in Lab 4 once you’ve
> finished the whole TCP implementation. As long as you pass those tests 100% and your
> implementation is reasonable, you’ll be fine

以下是“未完成时间过长”的规则。 你将要实现这个逻辑，它有点详细，但我们不希望你担心隐藏的测试用例试图绊倒你把这个当成 SAT 上的一个单词问题。

我们将在本周为您提供一些合理的单元测试，并在您完成整个 TCP 实现后在实验室 4 中进行更完整的集成测试。只要你 100% 通过这些测试并且你的实现是合理的，你就会没事的。

1. Every few milliseconds, your TCPSender’s tick method will be called with an argument
   that tells it how many milliseconds have elapsed since the last time the method was
   called. Use this to maintain a notion of the total number of milliseconds the TCPSender
   has been alive. Please don’t try to call any “time” or “clock” functions from
   the operating system or CPU—the tick method is your only access to the passage of
   time. That keeps things deterministic and testable.

2. When the TCPSender is constructed, it’s given an argument that tells it the “initial value”
   of the retransmission timeout (RTO). The RTO is the number of milliseconds to
   wait before resending an outstanding TCP segment. The value of the RTO will change
   over time, but the “initial value” stays the same. The starter code saves the “initial
   value” of the RTO in a member variable called initial retransmission timeout.

3. You’ll implement the retransmission timer: an alarm that can be started at a certain
   time, and the alarm goes off (or “expires”) once the RTO has elapsed. We emphasize
   that this notion of time passing comes from the tick method being called—not by
   getting the actual time of day.

   >您将实现重传计时器：一个可以在特定时间启动的警报，一旦 RTO 结束，警报就会响起（或“过期”）。我们强调，时间流逝的概念来自于被调用的 tick 方法——而不是通过获取一天中的实际时间

4. Every time a segment containing data (nonzero length in sequence space) is sent
   (whether it’s the first time or a retransmission), if the timer is not running, start it
   running so that it will expire after RTO milliseconds (for the current value of RTO
   By “expire,” we mean that the time will run out a certain number of milliseconds in
   the future.

5. When all outstanding data has been acknowledged, stop the retransmission timer.

6. If tick is called and the retransmission timer has expired:
   (a) Retransmit the earliest (lowest sequence number) segment that hasn’t been fully
   acknowledged by the TCP receiver. You’ll need to be storing the outstanding
   segments in some internal data structure that makes it possible to do this.
   (b) If the window size is nonzero:
   i. Keep track of the number of consecutive retransmissions, and increment it
   because you just retransmitted something. Your TCPConnection will use this
   information to decide if the connection is hopeless (too many consecutive
   retransmissions in a row) and needs to be aborted.
   ii. Double the value of RTO. This is called “exponential backoff”—it slows down
   retransmissions on lousy networks to avoid further gumming up the works.
   (c) Reset the retransmission timer and start it such that it expires after RTO millisec-
   onds (taking into account that you may have just doubled the value of RTO!).

7. When the receiver gives the sender an ackno that acknowledges the successful receipt
   of new data (the ackno reflects an absolute sequence number bigger than any previous
   ackno):
   (a)  Set the RTO back to its “initial value.”
   (b)  If the sender has any outstanding data, restart the retransmission timer so that it
   will expire after RTO milliseconds (for the current value of RTO).
   (c)  Reset the count of “consecutive retransmissions” back to zero.
   We would suggest implementing the functionality of the retransmission timer in a separate
   class, but it’s up to you. If you do, please add it to the existing files (tcp sender.hh and
   tcp sender.cc).
