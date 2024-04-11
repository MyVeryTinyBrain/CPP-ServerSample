```
방을 생성하고, 방에 참가 가능한 서버 샘플입니다.  
모든 참가자들에게 패킷을 브로드캐스트하거나 특정 참가자에게만 패킷을 송신할 수 있습니다.  
`Test` 프로젝트에 이를 사용해 간단한 채팅 기능이 구현되어 있습니다.
```

## CRoom
* 최대 인원수를 설정해 생성 가능한 방입니다.
* 고정 업데이트 주기마다 TCP, UDP 소켓을 사용해 참가자들에게 패킷을 브로드캐스트 하거나 한 명에게 패킷을 전송합니다.
* `IOnRoomMemberChangedCallback`를 상속받은 클래스를 등록해 참가자 입장, 퇴장 이벤트를 수신할 수 있습니다.

## CRoomSocket
* `CRoom::JoinRoom` 함수를 사용해 방에 성공적으로 참가하면 반환받는 룸 소켓입니다.
* TCP 송신, 수신을 위한 네이티브 소켓 한 개, UDP 송신, 수신을 위한 네이티브 소켓 두 개를 보유해 TCP, UDP를 모두 사용해 패킷을 송신, 수신할 수 있습니다.
* `SendPacket` 함수를 사용해 룸 서버에 패킷을 전송합니다.
* 서버로부터 수신한 패킷을 큐에 담아 저장합니다. 저장된 패킷은 `BeginPacketUse`, `EndPacketUse` 함수 사이에서 `PopPacket` 함수를 통해 Pop 할 수 있습니다.
* `BeginPacketUse` 함수는 큐를 잠궈 수신 스레드가 패킷을 큐로 옮기지 못하도록 합니다. `EndPacketUse` 함수는 큐의 잠금을 해제합니다.
* `IOnRoomSocketMemberChangedCallback`를 상속받은 클래스를 등록해 참가자 입장, 퇴장 이벤트를 수신할 수 있습니다.

## CPacketHeader
* 패킷을 전송할 때 사용하는 패킷 옵션입니다.
* `DataType`에 `DataTypeConfig` 비트 마스크를 부여해 룸 서버에게 명령하거나, 수신받은 패킷 타입을 파악하는데 사용됩니다.
* `DataTypeConfig::BroadcastDataType`: 이 플래그를 포함해 서버에게 전송하면, 서버는 모든 참가자들에게 패킷을 전송합니다.
* `DataTypeConfig::SingleTargetDataType`: 이 플래그를 포함해 서버에게 전송하면, 서버는 특정 참가자에게 패킷을 전송합니다.
* `SingleTargetReceiverID`: 특정 참가자에게 전송할 때, 대상이 되는 참가자의 ID 입니다.

## CSocket
* 하나의 네이티브 TCP 혹은 UDP 소켓을 보유합니다.
* 수신 큐를 가지고 있으며, TCP, UDP 타입에 따라 다른 송신, 수신 방법을 사용합니다.

## ISocketReceiveBuffer
* `ISocketReceiveBuffer` 클래스를 상속받은 `CTCPSocketReceiveBuffer`, `CUDPSocketReceiveBuffer`가 존재합니다. 각각 TCP, UDP 소켓의 패킷 수신 동작이 구현되어 있습니다.
* 우선 수신한 패킷을 고정된 크기의 바이트 버퍼에 담은 후, 패킷으로 변환해 큐에 저장합니다. 고정된 패킷보다 큰 패킷을 수신하면, 바이트 버퍼의 크기를 키워 수신합니다.

## ISocketSendMethod
* `ISocketSendMethod` 클래스를 상속받은 `CTCPSocketSendMethod`, `CUDPSocketSendMethod`가 존재합니다. 각각 TCP, UDP 소켓의 송신 동작이 구현되어 있습니다.
