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

除了发送这些段之外，TCPSender 还必须跟踪其未完成的段，直到它们占用的序列号被完全确认。 TCPSender 的所有者会定期调用 TCPSender 的 tick 方法，指示时间的流逝。 TCPSender 负责查看其未完成的 TCPSegment 的集合，并确定最旧的发送段是否在没有确认的情况下未完成的时间过长（即，没有确认其所有序列号）。如果是，则需要重传（再次发送）。



> Here are the rules for what “outstanding for too long” means.2 You’re going to be implement-
> ing this logic, and it’s a little detailed, but we don’t want you to be worrying about hidden
> test cases trying to trip you up or treating this like a word problem on the SAT. We’ll give
> you some reasonable unit tests this week, and fuller integration tests in Lab 4 once you’ve
> finished the whole TCP implementation. As long as you pass those tests 100% and your
> implementation is reasonable, you’ll be fine

以下是“未完成时间过长”的规则。 你将要实现这个逻辑，它有点详细，但我们不希望你担心隐藏的测试用例试图绊倒你把这个当成 SAT 上的一个单词问题。我们将在本周为您提供一些合理的单元测试，并在您完成整个 TCP 实现后在实验室 4 中进行更完整的集成测试。只要你 100% 通过这些测试并且你的实现是合理的，你就会没事的。



